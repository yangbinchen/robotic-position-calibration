/*******************************************************************************
*   Copyright 2013-2014 EPFL                                                   *
*   Copyright 2013-2014 Quentin Bonnard                                        *
*                                                                              *
*   This file is part of chilitags.                                            *
*                                                                              *
*   Chilitags is free software: you can redistribute it and/or modify          *
*   it under the terms of the Lesser GNU General Public License as             *
*   published by the Free Software Foundation, either version 3 of the         *
*   License, or (at your option) any later version.                            *
*                                                                              *
*   Chilitags is distributed in the hope that it will be useful,               *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of             *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
*   GNU Lesser General Public License for more details.                        *
*                                                                              *
*   You should have received a copy of the GNU Lesser General Public License   *
*   along with Chilitags.  If not, see <http://www.gnu.org/licenses/>.         *
*******************************************************************************/

#include <chilitags.hpp>

#include <opencv2/core/utility.hpp> // getTickCount...
#include <opencv2/imgproc/imgproc.hpp>


#include <opencv2/core/core_c.h> // CV_AA

#include <opencv2/highgui/highgui.hpp>

#include <iostream>

// Not much interesting here, move along
void drawTags(
    cv::Mat outputImage,
    const chilitags::TagCornerMap &tags,
    int64 startTime,
    int64 endTime,
    bool detection
    );

void logTagPosition(
  cv::Mat outputImage,
  const chilitags::TagCornerMap &tags,
  int64 startTime,
  int64 endTime
);

//   RelativeChilitags();
 // ~RelativeChilitags ();
class RelativeChilitags {
private:
  chilitags::TagCornerMap tags;
  // Set x and y calibration from 0 - 1 - 2
  // static const float CALIBRATION_X = 1.0f;
  // static const float CALIBRATION_Y = 1.0f;

public:
  RelativeChilitags ();
  void updateCornerMap(chilitags::TagCornerMap &newTags);
  cv::Point2f averagePos(cv::Mat_<cv::Point2f> tagCorners);
  cv::Point2f relPos (std::pair <int, chilitags::Quad> tag);
};

RelativeChilitags::RelativeChilitags () {
  // cv::Matx<float, 4, 2> q(0.0, 0.0,0.0, 0.0,0.0, 0.0, 0.0, 0.0);
  // chilitags::TagCornerMap m({0,q});
  // tags = m;

  // Initialize private tag variable
  chilitags::Quad q(0.0, 0.0,0.0, 0.0,0.0, 0.0, 0.0, 0.0);
  std::map<int, chilitags::Quad> tags = {
    std::pair<int, chilitags::Quad> (0,q),
  };
}

void RelativeChilitags::updateCornerMap(chilitags::TagCornerMap &newTags){
  if (newTags.size() >= tags.size()) {
    tags = newTags;
    std::cout<<"Updating tags \n";
  }
};

cv::Point2f RelativeChilitags::averagePos(cv::Mat_<cv::Point2f> tagCorners){
  cv::Point2f result = cv::Point2f(0.0, 0.0);
  for (int i = 0; i < 4; ++i){
      result += tagCorners(i) * 0.25;
  }
  return result;
}

cv::Point2f RelativeChilitags::relPos(std::pair <int, chilitags::Quad> tag){
  if (tag.first != 0 && tag.first != 1 && tag.first !=2) {
    const cv::Mat_<cv::Point2f> corners(tag.second);
    // Calculate the relative position from 0,0, using tags
    // ToDo - use the Calibrated x and y values, and 1 and 2 locations to define distances
    // ToDo implement this as a matrix transformation
    // ToDo implement orientation calculation also
    return cv::Point2f(1.0, 1.0);
  } else {
    // Cannot have a relative position from one of the relative tags
    return cv::Point2f(0.0, 0.0);
  }
}


int main(int argc, char* argv[])
{
    // Initialising input video - basic options
    int xRes = 640;
    int yRes = 480;
    int cameraIndex = 0;
    // Below options work for USB webcam
    // int xRes = 1280;
    // int yRes = 720;
    // int cameraIndex = 4;

    if (argc > 2) {
        xRes = std::atoi(argv[1]);
        yRes = std::atoi(argv[2]);
    }
    if (argc > 3) {
        cameraIndex = std::atoi(argv[3]);
    }

    // The source of input images
    cv::VideoCapture capture(cameraIndex);
    if (!capture.isOpened())
    {
        std::cerr << "Unable to initialise video capture." << std::endl;
        return 1;
    }
    capture.set(cv::CAP_PROP_FRAME_WIDTH, xRes);
    capture.set(cv::CAP_PROP_FRAME_HEIGHT, yRes);

    cv::Mat inputImage;

    // We need separate Chilitags if we want to compare find() with different
    // detection/tracking parameters on the same image

    RelativeChilitags r;
    // This one is the reference Chilitags
    chilitags::Chilitags detectedChilitags;
    detectedChilitags.setFilter(0, 0.0f);


    cv::namedWindow("DisplayChilitags");

    // Do we want to run and show the reference detection ?
    bool showReference = true;
    // Do we want to run and show the tracking-based detection ?


    char keyPressed;
    while ('q' != (keyPressed = (char) cv::waitKey(1))) {

        // toggle the processing, according to user input
        if (keyPressed == 'd') showReference = !showReference;
        capture.read(inputImage);
        cv::Mat outputImage = inputImage.clone();
        int64 startTime = cv::getTickCount();
        auto tags = detectedChilitags.find(inputImage);
        int64 endTime = cv::getTickCount();
        r.updateCornerMap(tags);
        // drawTags(outputImage, tags, startTime, endTime, true);
        logTagPosition(outputImage, tags, startTime, endTime);

        cv::imshow("DisplayChilitags", outputImage);
    }

    cv::destroyWindow("DisplayChilitags");
    capture.release();

    return 0;
}





void logTagPosition(
  cv::Mat outputImage,
  const chilitags::TagCornerMap &tags,
  int64 startTime,
  int64 endTime){
      // Print in green
    cv::Scalar COLOR = cv::Scalar(0, 255, 0);
    // loop through tags:
    for (const auto & tag : tags) {

        const cv::Mat_<cv::Point2f> corners(tag.second);
        std::cout<<"Tag: "<< tag.first <<"\n";
        for (size_t i = 0; i < 4; ++i) {
            static const int SHIFT = 16;
            static const float PRECISION = 1<<SHIFT;
            std::cout<< corners(i)<<"\n";
            cv::line(
                outputImage,
                PRECISION*corners(i),
                PRECISION*corners((i+1)%4),
                COLOR, 1, cv::LINE_AA, SHIFT);
            // removing the shift & precision stuff doesn't plot a correct square.
        }

        cv::Point2f center = 0.5f*(corners(0) + corners(2));
        if (tag.first == 42 ) {
          cv::putText(outputImage, cv::format("aaa%d", tag.first), center,
                      cv::FONT_HERSHEY_SIMPLEX, 0.5, COLOR);
          /* code */
        }else{
          cv::putText(outputImage, cv::format("%d", tag.first), center,
                      cv::FONT_HERSHEY_SIMPLEX, 0.5, COLOR);
        };

    }

    float processingTime = 1000.0f*((float) endTime - startTime)/cv::getTickFrequency();
    cv::putText(outputImage,
                cv::format("%dx%d %4.0f ms (press '%c' to toggle %s)",
                           outputImage.cols, outputImage.rows,
                           processingTime,
                           'd',
                           "simple detection"
                           ),
                cv::Point(32,32),
                cv::FONT_HERSHEY_SIMPLEX, 0.5f, COLOR);
}


void drawTags(
    cv::Mat outputImage,
    const chilitags::TagCornerMap &tags,
    int64 startTime,
    int64 endTime,
    bool detection
    ){
    cv::Scalar COLOR = detection ?
                       cv::Scalar(0, 0, 255) :
                       cv::Scalar(255, 0, 0);

    for (const auto & tag : tags) {

        const cv::Mat_<cv::Point2f> corners(tag.second);

        for (size_t i = 0; i < 4; ++i) {
            static const int SHIFT = 16;
            static const float PRECISION = 1<<SHIFT;
            cv::line(
                outputImage,
                PRECISION*corners(i),
                PRECISION*corners((i+1)%4),
                COLOR, detection ? 3 : 1, cv::LINE_AA, SHIFT);

        }

        cv::Point2f center = 0.5f*(corners(0) + corners(2));
        if (tag.first == 42 ) {
          cv::putText(outputImage, cv::format("aaa%d", tag.first), center,
                      cv::FONT_HERSHEY_SIMPLEX, 0.5, COLOR);
          /* code */
        }else{
          cv::putText(outputImage, cv::format("%d", tag.first), center,
                      cv::FONT_HERSHEY_SIMPLEX, 0.5, COLOR);
        };

    }

    float processingTime = 1000.0f*((float) endTime - startTime)/cv::getTickFrequency();
    cv::putText(outputImage,
                cv::format("%dx%d %4.0f ms (press '%c' to toggle %s)",
                           outputImage.cols, outputImage.rows,
                           processingTime,
                           detection ? 'd' : 't',
                           detection ? "simple detection" : "tracking"
                           ),
                cv::Point(32,detection ? 32 : 64),
                cv::FONT_HERSHEY_SIMPLEX, 0.5f, COLOR);
}
