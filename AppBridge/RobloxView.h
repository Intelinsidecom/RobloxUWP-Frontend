// AppBridge\RobloxView.h
// Source path (from PDB): C:\p4v\Branches\WinUWP_dev\Client\AppBridge\RobloxView.h


#pragma once

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>

#include "v8datamodel/Game.h"
#include "rbx/signal.h"
#include "v8tree/Verb.h"

namespace RBX
{
    class DataModel;
    class ViewBase;
    class FunctionMarshaller;

    namespace Tasks
    {
        class Sequence;
    }

    namespace Reflection
    {
        class PropertyDescriptor;
    }

    class LeaveGameVerb : public RBX::Verb
    {
    public:
        LeaveGameVerb(RBX::VerbContainer* container);
        virtual ~LeaveGameVerb();
        virtual void doIt(RBX::IDataState* dataState);
    };
}

class RobloxView
{
    boost::scoped_ptr<class RBX::LeaveGameVerb> leaveGameVerb;
    RBX::ViewBase* view;
    boost::shared_ptr<RBX::Tasks::Sequence> sequence;
    boost::shared_ptr<RBX::Game> game;
    RBX::FunctionMarshaller* marshaller;
    rbx::signals::scoped_connection placeIDChangeConnection;

    class RenderJob;
    boost::shared_ptr<RenderJob> renderJob;

    void onPlaceIDChanged(const RBX::Reflection::PropertyDescriptor* desc);
public:
    RobloxView(void* wnd, unsigned int width, unsigned int height);
    ~RobloxView(void);

    void requestStopRenderingForBackgroundMode();

    void exitGame();

    void replaceGame(shared_ptr<RBX::Game> game);

    static RobloxView* create_view(shared_ptr<RBX::Game> game, void* wnd, unsigned int width, unsigned int height);

    boost::shared_ptr<RBX::DataModel> getDataModel() { return game->getDataModel(); }
    boost::shared_ptr<RBX::Game> getGame() { return game; }
    RBX::ViewBase* getView() { return view; }

private:
    unsigned int width;
    unsigned int height;

    void defineConcurrencyRules();
};
