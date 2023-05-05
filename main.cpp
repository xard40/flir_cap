#include <chrono>
#include <csignal>
#include <opencv2/opencv.hpp>
#include "appHandler.h"
#include "spinCamera.h"

std::unique_ptr<cAppHandler> pAppHandler = nullptr;
volatile bool isRunning = true;
volatile bool isSave = false;

static void sigHandler(int signum) {
  isRunning = false;
}

static std::unique_ptr<cAppHandler> initAPP(std::string configFile) {
  char current_wd[1024];
  getcwd(current_wd, sizeof(current_wd));
  std::unique_ptr<cAppHandler> appHandler = std::make_unique<cAppHandler>(current_wd);
  int configVideoNumber = appHandler->InitHandler(configFile);
  if (configVideoNumber <= 0) {
    appHandler = nullptr;
  }
  return appHandler;
}

int main(int argc, char** argv) {
  signal(SIGINT, sigHandler);

  std::string configFile = (argc < 2) ? "./config/flirConfig.yaml" : argv[1];
  pAppHandler = initAPP(configFile);
  if (!pAppHandler) {
    return -1;
  }

  while (isRunning) {
    pAppHandler->GoHandler();
    char key = cv::waitKey(1);
    if (key == 27 || key == 'q') break;
    if (key == 's') cv::waitKey(0);
  }

  pAppHandler->ExitHandler();
  return 0;
}
