#include <icons.h>

#define STB_IMAGE_IMPLEMENTATION
#include <imgui/stb_image.h>

namespace icons {
    ImTextureID LOGO;
    ImTextureID PLAY;
    ImTextureID STOP;
    ImTextureID MENU;

    GLuint loadTexture(std::string path) {
        int w,h,n;
        stbi_uc* data = stbi_load(path.c_str(), &w, &h, &n, NULL);
        GLuint texId;
        glGenTextures(1, &texId);
        glBindTexture(GL_TEXTURE_2D, texId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, (uint8_t*)data);
        stbi_image_free(data);
        return texId;
    }

    void load() {
        LOGO = (ImTextureID)loadTexture(config::getRootDirectory() + "/res/icons/sdrpp.png");
        PLAY = (ImTextureID)loadTexture(config::getRootDirectory() + "/res/icons/play.png");
        STOP = (ImTextureID)loadTexture(config::getRootDirectory() + "/res/icons/stop.png");
        MENU = (ImTextureID)loadTexture(config::getRootDirectory() + "/res/icons/menu.png");
    }
}