#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

class $modify(TransparentScreenshotPlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool useDownloadedLevel, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useDownloadedLevel, dontCreateObjects)) {
            return false;
        }
        return true;
    }

    void keyDown(enumKeyCodes key) {
        PlayLayer::keyDown(key);

        // Capture screenshot on F9 keypress
        if (key == enumKeyCodes::KEY_F9) {
            auto director = CCDirector::sharedDirector();
            auto winSize = director->getWinSize();

            // Create a render texture with RGBA buffer for transparency
            auto texture = CCRenderTexture::create(
                static_cast<int>(winSize.width), 
                static_cast<int>(winSize.height), 
                CCTexture2D::PixelFormat::RGBA8888
            );

            if (texture) {
                texture->begin();
                this->visit();
                texture->end();

                // Save to the Geode mod's save directory
                std::string path = Mod::get()->getSaveDir().string() + "/transparent_screenshot.png";
                
                if (texture->saveToFile(path.c_str(), true)) {
                    FLAlertLayer::create("Screenshot Saved", "Saved transparent screenshot to your Geode mod save folder!", "OK")->show();
                } else {
                    FLAlertLayer::create("Error", "Failed to save screenshot file.", "OK")->show();
                }
            }
        }
    }
};
