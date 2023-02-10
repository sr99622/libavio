#include "avio.h"

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cout << "Usage: sample-cmd filename" << std::endl;
        return 1;
    }

    std::cout << "playing file: " << argv[1] << std::endl;
    
    try {
        std::function<void(const std::string& arg)> errorCallback = [&](const std::string& arg)
        {
            std::cout << "msg: " << arg << std::endl;
        };

        avio::Player player;

        player.uri = argv[1];
        player.video_filter = "scale=1280x720";
        player.errorCallback = errorCallback;
        player.infoCallback = errorCallback;

        player.run();
    }
    catch (const avio::Exception& e) {
        std::cout << "error: " << e.what() << std::endl;
    }


    return 0;
}