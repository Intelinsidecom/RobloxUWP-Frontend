// AppBridge\RobloxView.cpp
// Source path (from PDB): C:\p4v\Branches\WinUWP_dev\Client\AppBridge\RobloxView.cpp

#include "pch.h"
#include "RobloxView.h"

#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/thread.hpp>

#include "v8datamodel/BaseRenderJob.h"
#include "v8datamodel/DataModel.h"
#include "v8datamodel/Game.h"
#include "rbx/rbxTime.h"
#include "rbx/Profiler.h"

#include "util/IMetric.h"
#include "util/standardout.h"
#include "rbx/SystemUtil.h"
#include "v8tree/Verb.h"

#include "GfxBase/ViewBase.h"
#include "GfxBase/FrameRateManager.h"
#include "GfxBase/RenderSettings.h"

#include "ClientBase/RenderSettingsItem.h"
#include "rbx/RbxFormat.h"

#include "FastLog.h"
#include "RbxAssert.h"
#include "FunctionMarshaller.h"

FASTFLAGVARIABLE(RenderCleanupInBackground, true)
FASTLOG(FLog::RenderBreakdown, "Trigger renderPrepare");
FASTLOG(FLog::RenderBreakdown, "Finished renderPrepare");

namespace RBX
{
class IDataState;

boost::function<void()>& getLeaveGameCallback()
{
    static boost::function<void()> callback;
    return callback;
}

class RobloxView::RenderJob : public RBX::BaseRenderJob, public RBX::IMetric
{
public:
    RenderJob(RBX::ViewBase* view, RBX::FunctionMarshaller* marshaller, boost::shared_ptr<RBX::DataModel> dataModel);
    virtual ~RenderJob();

    void stop();

    virtual RBX::TaskScheduler::StepResult stepDataModelJob(const Stats& stats);
    virtual RBX::Time::Interval sleepTime(const Stats& stats);

    virtual std::string getMetric(const std::string& metric) const;
    virtual double getMetricValue(const std::string& metric) const;

private:
    void wake() { SetEvent(renderEvent); }

    RBX::FunctionMarshaller* marshaller;
    boost::shared_ptr<RBX::DataModel> dataModel;
    RBX::ViewBase* view;
    HANDLE renderEvent;
    HANDLE stoppedEvent;
    bool stopped;
};

static RBX::ViewBase* createGameWindow(void* wnd, unsigned int width, unsigned int height)
{
    CRenderSettingsItem& settings = CRenderSettingsItem::singleton();

    static const RBX::CRenderSettings::GraphicsMode modes[] =
    {
        RBX::CRenderSettings::Direct3D11,
        RBX::CRenderSettings::Direct3D9,
        RBX::CRenderSettings::OpenGL,
    };

    for (size_t i = 0; i < sizeof(modes) / sizeof(modes[0]); i++)
    {
        RBX::CRenderSettings::GraphicsMode mode = modes[i];

        try
        {
            RBX::OSContext context;
            context.hWnd = wnd;
            context.width = width;
            context.height = height;

            RBX::ViewBase* rbxView = RBX::ViewBase::CreateView(mode, &context, &settings);
            rbxView->initResources();

            StandardOut::singleton()->printf(MESSAGE_INFO, "View created with graphics mode %d", mode);
            return rbxView;
        }
        catch (std::exception& e)
        {
            StandardOut::singleton()->printf(MESSAGE_ERROR, "Mode %d failed: \"%s\"", mode, e.what());
        }
    }

    RBXASSERT(0);
    throw RBX::runtime_error("GraphicsInitErrorNoModes");
}

RobloxView::RobloxView(void* wnd, unsigned int width, unsigned int height)
    : leaveGameVerb(NULL)
    , view(createGameWindow(wnd, width, height))
    , sequence()
    , game()
    , marshaller(FunctionMarshaller::GetWindow())
    , placeIDChangeConnection()
    , renderJob()
    , width(width)
    , height(height)
{
    RBXASSERT(view);
}

RobloxView::~RobloxView()
{
    if (renderJob)
    {
        renderJob->stop();
        renderJob.reset();
    }
    leaveGameVerb.reset();
    view.reset();
}

RobloxView* RobloxView::create_view(shared_ptr<RBX::Game> game, void* wnd, unsigned int width, unsigned int height)
{
    boost::scoped_ptr<RobloxView> view(new RobloxView(wnd, width, height));
    view->replaceGame(game);
    getLeaveGameCallback() = boost::bind(&RobloxView::exitGame, view.get());

    return view.release();
}

void RobloxView::replaceGame(shared_ptr<RBX::Game> game)
{
    this->game = game;
    placeIDChangeConnection.disconnect();
    placeIDChangeConnection = game->getDataModel()->propertyChangedSignal.connect(
        boost::bind(&RobloxView::onPlaceIDChanged, this, _1));

    if (renderJob)
    {
        renderJob->stop();
        renderJob.reset();
    }

    boost::shared_ptr<RBX::DataModel> dataModel = game->getDataModel();
    renderJob.reset(new RenderJob(view.get(), marshaller, dataModel));
    leaveGameVerb.reset(new RBX::LeaveGameVerb(game->getDataModel().get()));

    defineConcurrencyRules();
}

void RobloxView::defineConcurrencyRules()
{
    RBXASSERT(renderJob);
}

void RobloxView::requestStopRenderingForBackgroundMode()
{
    if (!renderJob)
        return;

    renderJob->stop();

    if (FFlag::RenderCleanupInBackground)
    {
        marshaller->ProcessMessages();
    }
    else
    {
        renderJob.reset();
    }
}

void RobloxView::exitGame()
{
    if (renderJob)
        renderJob->stop();
}

void RobloxView::onPlaceIDChanged(const RBX::Reflection::PropertyDescriptor* desc)
{
}

RobloxView::RenderJob::RenderJob(RBX::ViewBase* view, RBX::FunctionMarshaller* marshaller, boost::shared_ptr<RBX::DataModel> dataModel)
    : RBX::BaseRenderJob(30.0f, 60.0f, boost::shared_ptr<RBX::DataModel>(dataModel, [](RBX::DataModel*){}))
    , marshaller(marshaller)
    , dataModel(dataModel)
    , view(view)
    , renderEvent(CreateEvent(NULL, FALSE, FALSE, NULL))
    , stoppedEvent(CreateEvent(NULL, FALSE, FALSE, NULL))
    , stopped(false)
{
}

RobloxView::RenderJob::~RenderJob()
{
    stop();
    CloseHandle(renderEvent);
    CloseHandle(stoppedEvent);
}

void RobloxView::RenderJob::stop()
{
    stopped = true;
    SetEvent(stoppedEvent);
}

RBX::TaskScheduler::StepResult RobloxView::RenderJob::stepDataModelJob(const Stats& stats)
{
    RBXPROFILER_SCOPE("Jobs", __FUNCTION__);

    if (stopped || !view || !view->getFrameRateManager())
        return RBX::TaskScheduler::Stepped;

    double seconds = 0.016;
    {
        boost::shared_ptr<RBX::DataModel> dmPtr = boost::static_pointer_cast<RBX::DataModel>(dataModel->shared_from_this());
        RBX::DataModel::scoped_write_request request(dmPtr.get());

        dmPtr->renderStep(seconds);
        seconds = RBX::Time::nowFastSec();

        view->renderPrepare(this);
    }

    view->renderPerform(seconds);
    wake();

    return RBX::TaskScheduler::Stepped;
}

RBX::Time::Interval RobloxView::RenderJob::sleepTime(const Stats& stats)
{
    if (WaitForSingleObject(stoppedEvent, 0) != WAIT_OBJECT_0)
        return RBX::Time::Interval::max();
    return RBX::BaseRenderJob::computeStandardSleepTime(stats, maxFrameRate);
}

std::string RobloxView::RenderJob::getMetric(const std::string& metric) const
{
    if (!view)
        return "No View";

    if (metric.size() == 13 && strncmp(metric.c_str(), "Graphics Mode", 13) == 0)
        return "";

    RBX::FrameRateManager* frm = view->getFrameRateManager();

    if (metric == "FRM")
        return (frm && frm->IsBlockCullingEnabled()) ? "On" : "Off";

    if (metric == "Anti-Aliasing")
    {
        RBXASSERT(0);
        return "";
    }

    return "";
}

double RobloxView::RenderJob::getMetricValue(const std::string& metric) const
{
    if (!view)
        return 0.0;

    RBX::FrameRateManager* frm = view->getFrameRateManager();

    if (metric.size() == 10 && strncmp(metric.c_str(), "Render FPS", 10) == 0)
        return averageStepsPerSecond();
    if (metric.size() == 11 && strncmp(metric.c_str(), "Render Duty", 11) == 0)
        return averageDutyCycle();
    if (metric.size() == 15 && strncmp(metric.c_str(), "Render Job Time", 15) == 0)
        return averageStepTime();

    if (metric == "Render Nominal FPS")
        return frm ? 1000.0 / frm->GetRenderTimeAverage() : 0.0;
    if (metric == "Delta Between Renders" || metric == "Total Render" ||
        metric == "Present Time" || metric == "GPU Delay" || metric == "Render Prepare")
        return view->getMetricValue(metric);

    if (metric == "Video Memory MB")
        return RBX::SystemUtil::getVideoMemory() / 1e6;

    return 0.0;
}

RBX::LeaveGameVerb::LeaveGameVerb(RBX::VerbContainer* container)
    : RBX::Verb(container, "Exit")
{
}

RBX::LeaveGameVerb::~LeaveGameVerb()
{
}

void RBX::LeaveGameVerb::doIt(RBX::IDataState* dataState)
{
    RBX::FunctionMarshaller* marshaller = RBX::FunctionMarshaller::GetWindow();
    if (!marshaller)
        return;

    boost::function<void()> exitCallback = getLeaveGameCallback();
    if (exitCallback)
        marshaller->Execute(exitCallback);
}

} // namespace RBX
