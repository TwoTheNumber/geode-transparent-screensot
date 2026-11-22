#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>
#include <ctime>
#include <filesystem>

using namespace geode::prelude;

// Returns the folder chosen in mod settings, or the default save folder if none is set
std::filesystem::path getOutputFolder() {
    auto folder = Mod::get()->getSettingValue<std::filesystem::path>("output-folder");
    if (!folder.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(folder, ec);
        if (!ec) return folder;
        log::warn("Could not create output folder {}: {}", folder.string(), ec.message());
    }
    return Mod::get()->getSaveDir();
}

// Returns "png" or "jpg" based on the file type setting
std::string getFileExtension() {
    auto type = Mod::get()->getSettingValue<std::string>("file-type");
    return type == "JPG" ? "jpg" : "png";
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

    // Build the output path from the folder and file type settings
    auto ext = getFileExtension();
    auto timestamp = std::time(nullptr);
    auto filename = fmt::format("transparent_capture_{}.{}", timestamp, ext);
    auto path = getOutputFolder() / filename;
    auto pathStr = path.generic_string();
    log::info("Saving capture to {}", pathStr);

    // Get the rendered pixels as an image, then save that image to disk
    auto image = renderTexture->newCCImage(true);
    if (image) {
        bool saved = image->saveToFile(pathStr.c_str(), false);
        image->release();
        log::info("CCImage::saveToFile returned: {}", saved);
    } else {
        log::error("newCCImage returned null");
    }

    bool written = std::filesystem::exists(path);
    log::info("File exists after save: {}", written);

    if (written) {
        Notification::create("Saved transparent image!", NotificationIcon::Success)->show();
    } else {
        log::error("Capture was not written to {}", pathStr);
        Notification::create("Failed to save image, see log", NotificationIcon::Error)->show();
    }
}

// Press F9 inside any level to capture
class $modify(MyKeyboardDispatcher, CCKeyboardDispatcher) {
    bool dispatchKeyboardMSG(enumKeyCodes key, bool down, bool repeat, double timestamp) {
        if (down && !repeat && key == enumKeyCodes::KEY_F9) {
            auto playLayer = PlayLayer::get();
            log::info("F9 pressed, PlayLayer present: {}", playLayer != nullptr);
            if (playLayer) {
                captureTransparentLevel(playLayer);
                return true;
            }
        }
        return CCKeyboardDispatcher::dispatchKeyboardMSG(key, down, repeat, timestamp);
    }
};
