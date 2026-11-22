#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>
#include <ctime>
#include <filesystem>

using namespace geode::prelude;

// Returns the folder set in mod settings, or the default save folder if it's empty
std::filesystem::path getOutputFolder() {
    auto custom = Mod::get()->getSettingValue<std::string>("output-folder");
    if (!custom.empty()) {
        std::filesystem::path folder(custom);
        std::error_code ec;
        std::filesystem::create_directories(folder, ec);
        if (!ec) return folder;
    }
    return Mod::get()->getSaveDir();
}

void captureTransparentLevel(PlayLayer* playLayer) {
    if (!playLayer) return;

    auto director = CCDirector::sharedDirector();
    auto winSize = director->getWinSize();

    // Save current visibility states
    bool bgVisible = playLayer->m_background ? playLayer->m_background->isVisible() : true;
    bool groundVisible = playLayer->m_groundLayer ? playLayer->m_groundLayer->isVisible() : true;
    bool uiVisible = playLayer->m_uiLayer ? playLayer->m_uiLayer->isVisible() : true;

    // Hide background, ground, and UI so only objects, player, and trails render
    if (playLayer->m_background) playLayer->m_background->setVisible(false);
    if (playLayer->m_groundLayer) playLayer->m_groundLayer->setVisible(false);
    if (playLayer->m_uiLayer) playLayer->m_uiLayer->setVisible(false);

    // Create an RGBA render texture (supports variable alpha transparency)
    auto renderTexture = CCRenderTexture::create(
        static_cast<int>(winSize.width),
        static_cast<int>(winSize.height),
        kCCTexture2DPixelFormat_RGBA8888
    );

    // Clear with 0 alpha and draw the level
    renderTexture->beginWithClear(0, 0, 0, 0);
    playLayer->visit();
    renderTexture->end();

    // Restore visibility immediately
    if (playLayer->m_background) playLayer->m_background->setVisible(bgVisible);
    if (playLayer->m_groundLayer) playLayer->m_groundLayer->setVisible(groundVisible);
    if (playLayer->m_uiLayer) playLayer->m_uiLayer->setVisible(uiVisible);

    // Save PNG to the configured output folder
    auto timestamp = std::time(nullptr);
    auto filename = fmt::format("transparent_capture_{}.png", timestamp);
    auto path = getOutputFolder() / filename;

    renderTexture->saveToFile(path.string().c_str(), kCCImageFormatPNG);

    Notification::create("Saved transparent PNG!", NotificationIcon::Success)->show();
}

// Press F9 inside any level to capture
class $modify(MyKeyboardDispatcher, CCKeyboardDispatcher) {
    bool dispatchKeyboardMSG(enumKeyCodes key, bool down, bool repeat, double timestamp) {
        if (down && !repeat && key == enumKeyCodes::KEY_F9) {
            if (auto playLayer = PlayLayer::get()) {
                captureTransparentLevel(playLayer);
                return true;
            }
        }
        return CCKeyboardDispatcher::dispatchKeyboardMSG(key, down, repeat, timestamp);
    }
};
