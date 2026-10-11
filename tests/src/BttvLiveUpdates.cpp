// SPDX-FileCopyrightText: 2023 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "providers/bttv/BttvLiveUpdates.hpp"

#include "mocks/BaseApplication.hpp"
#include "providers/bttv/liveupdates/BttvLiveUpdateClient.hpp"
#include "providers/bttv/liveupdates/BttvLiveUpdateMessages.hpp"
#include "providers/liveupdates/BasicPubSubManager.hpp"
#include "providers/liveupdates/Diag.hpp"
#include "Test.hpp"

#include <QScopeGuard>
#include <QString>
#include <QtCore/qtestsupport_core.h>

#include <memory>
#include <mutex>
#include <optional>
#include <tuple>
#include <vector>

using namespace chatterino;
using namespace std::chrono_literals;

const QString TARGET_USER_ID = "1234567";
const QString TARGET_USER_NAME = "Alien";

namespace {

// Exercise the real BTTV send hooks against the existing local echo server.
class RecordingClient : public BttvLiveUpdateClient
{
public:
    RecordingClient(BttvLiveUpdates &manager, std::mutex &mutex,
                    std::vector<QByteArray> &messages)
        : BttvLiveUpdateClient(manager)
        , mutex_(mutex)
        , messages_(messages)
    {
    }

    void onMessage(const QByteArray &msg)
    {
        std::lock_guard lock(this->mutex_);
        this->messages_.push_back(msg);
    }

private:
    std::mutex &mutex_;
    std::vector<QByteArray> &messages_;
};

class RecordingManager
    : public BasicPubSubManager<RecordingManager, RecordingClient>
{
public:
    RecordingManager()
        : BasicPubSubManager("ws://" + PUBSUB_WS_ADDR + "/echo", "BTTV test")
        , unusedManager_("ws://" + PUBSUB_WS_ADDR + "/echo")
    {
    }

    ~RecordingManager() override
    {
        this->stop();
    }

    std::shared_ptr<RecordingClient> makeClient()
    {
        return std::make_shared<RecordingClient>(this->unusedManager_,
                                                 this->mutex_, this->messages_);
    }

    using BasicPubSubManager::subscribe;
    using BasicPubSubManager::unsubscribe;

    void broadcast(const QString &userID)
    {
        this->clients().begin()->second->broadcastMe(TARGET_USER_ID, userID);
    }

    void disconnect()
    {
        this->clients().begin()->second->sendText("/CLOSE");
    }

    std::vector<QByteArray> messages()
    {
        std::lock_guard lock(this->mutex_);
        return this->messages_;
    }

private:
    BttvLiveUpdates unusedManager_;
    std::mutex mutex_;
    std::vector<QByteArray> messages_;
};

}  // namespace

TEST(BttvLiveUpdates, ActivityConsent)
{
    mock::BaseApplication app;
    ASSERT_FALSE(app.settings.sendBTTVActivity);
    const auto restoreConsent = qScopeGuard([&] {
        app.settings.sendBTTVActivity = false;
    });
    RecordingManager manager;
    const BttvLiveUpdateSubscription channel{
        BttvLiveUpdateSubscriptionChannel{TARGET_USER_ID}};
    const BttvLiveUpdateSubscription presence{
        BttvLiveUpdateBroadcastMe{TARGET_USER_ID, TARGET_USER_NAME}};

    // Consent can change while the initial presence waits for connection open.
    app.settings.sendBTTVActivity = true;
    manager.subscribe(presence);
    manager.subscribe(channel);
    app.settings.sendBTTVActivity = false;
    ASSERT_TRUE(QTest::qWaitFor([&] {
        return manager.messages().size() == 1;
    }));
    EXPECT_TRUE(manager.messages().front().contains("join_channel"));

    manager.broadcast(TARGET_USER_NAME);
    QTest::qWait(50);
    EXPECT_EQ(manager.messages().size(), 1);

    app.settings.sendBTTVActivity = true;
    manager.broadcast(TARGET_USER_NAME);
    ASSERT_TRUE(QTest::qWaitFor([&] {
        return manager.messages().size() == 2;
    }));
    EXPECT_TRUE(manager.messages().back().contains("broadcast_me"));
    EXPECT_TRUE(manager.messages().back().contains(TARGET_USER_NAME.toUtf8()));

    // Disabling consent must suppress direct sends and cached reconnect replay.
    app.settings.sendBTTVActivity = false;
    manager.broadcast(TARGET_USER_NAME);
    manager.disconnect();
    ASSERT_TRUE(QTest::qWaitFor([&] {
        return manager.diag.connectionsOpened == 2;
    }));
    ASSERT_TRUE(QTest::qWaitFor([&] {
        return manager.messages().size() == 3;
    }));
    EXPECT_TRUE(manager.messages().back().contains("join_channel"));

    app.settings.sendBTTVActivity = true;
    manager.broadcast({});  // anonymous users have no presence to report
    manager.unsubscribe(presence);  // must not re-report presence when leaving
    manager.unsubscribe(channel);
    ASSERT_TRUE(QTest::qWaitFor([&] {
        return manager.messages().size() == 4;
    }));
    EXPECT_TRUE(manager.messages().back().contains("part_channel"));
    QTest::qWait(50);
    EXPECT_EQ(manager.messages().size(), 4);
}

class BttvLiveUpdatesEvents : public ::testing::TestWithParam<bool>
{
};

TEST_P(BttvLiveUpdatesEvents, AllEvents)
{
    mock::BaseApplication app;
    ASSERT_FALSE(app.settings.sendBTTVActivity);
    app.settings.sendBTTVActivity = GetParam();
    const auto restoreConsent = qScopeGuard([&] {
        app.settings.sendBTTVActivity = false;
    });

    const QString host("wss://" + PUBSUB_WSS_ADDR +
                       "/liveupdates/bttv/all-events");
    chatterino::BttvLiveUpdates liveUpdates(host);

    std::optional<BttvLiveUpdateEmoteUpdateAddMessage> addMessage;
    std::optional<BttvLiveUpdateEmoteUpdateAddMessage> updateMessage;
    std::optional<BttvLiveUpdateEmoteRemoveMessage> removeMessage;

    std::ignore = liveUpdates.signals_.emoteAdded.connect([&](const auto &m) {
        addMessage = m;
    });
    std::ignore = liveUpdates.signals_.emoteUpdated.connect([&](const auto &m) {
        updateMessage = m;
    });
    std::ignore = liveUpdates.signals_.emoteRemoved.connect([&](const auto &m) {
        removeMessage = m;
    });

    liveUpdates.joinChannel(TARGET_USER_ID, TARGET_USER_NAME);
    QTest::qWait(500);

    ASSERT_EQ(liveUpdates.diag().connectionsOpened, 1);
    ASSERT_EQ(liveUpdates.diag().connectionsClosed, 0);
    ASSERT_EQ(liveUpdates.diag().connectionsFailed, 0);

    ASSERT_TRUE(addMessage.has_value());
    auto add = *addMessage;
    ASSERT_EQ(add.channelID, TARGET_USER_ID);
    ASSERT_EQ(add.emoteName, QString("PepePls"));
    ASSERT_EQ(add.emoteID, QString("55898e122612142e6aaa935b"));

    // The v1.0.12 fixture deliberately delays emote_update until broadcast_me.
    // Create/remove still exercise public subscriptions with activity disabled.
    if (GetParam())
    {
        ASSERT_TRUE(updateMessage.has_value());
        auto update = *updateMessage;
        ASSERT_EQ(update.channelID, TARGET_USER_ID);
        ASSERT_EQ(update.emoteName, QString("PepePls"));
        ASSERT_EQ(update.emoteID, QString("55898e122612142e6aaa935b"));
    }
    else
    {
        ASSERT_FALSE(updateMessage.has_value());
    }

    ASSERT_TRUE(removeMessage.has_value());
    auto rem = *removeMessage;
    ASSERT_EQ(rem.channelID, TARGET_USER_ID);
    ASSERT_EQ(rem.emoteID, QString("55898e122612142e6aaa935b"));

    liveUpdates.stop();
    // after exactly one event loop iteration, we should see updated counters
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    ASSERT_EQ(liveUpdates.diag().connectionsOpened, 1);
    ASSERT_EQ(liveUpdates.diag().connectionsClosed, 1);
    ASSERT_EQ(liveUpdates.diag().connectionsFailed, 0);
}

INSTANTIATE_TEST_SUITE_P(ActivitySettings, BttvLiveUpdatesEvents,
                         ::testing::Values(false, true));
