#include "esim/common/string_format.hpp"
#include "opencv2/imgcodecs.hpp"

#include <esim/esim/camera_simulator.hpp>
#include <fstream>
#include <iomanip>
#include <ostream>
#include <ze/common/file_utils.hpp>

#define FRAME_DIR_PATH "/home/arno/sim_ws/out/frames"

static uint frame_number = 0;
static std::ofstream exposures_file_;
static std::string output_folder = "/home/arno/sim_ws/out";

namespace event_camera_simulator {

    void ImageBuffer::addImage(Time t, const Image& img) {
        if (!data_.empty()) {
            // Check that the image timestamps are monotonically increasing
            CHECK_GT(t, data_.back().stamp);
        }

        Duration exposure_time = data_.empty() ? 0 : t - data_.back().stamp;
        VLOG(2) << "Adding image to buffer with stamp: " << t
                << " and exposure time " << exposure_time;
        data_.push_back(ImageData(img.clone(), t, exposure_time));

        // Remove all the images with timestamp older than t - buffer_size_ns_
        auto first_valid_element = std::lower_bound(
            data_.begin(),
            data_.end(),
            t - buffer_size_ns_,
            [](ImageData lhs, Time rhs) -> bool { return lhs.stamp < rhs; }
        );

        data_.erase(data_.begin(), first_valid_element);
        VLOG(3) << "first/last element in buffer: " << data_.front().stamp
                << " " << data_.back().stamp;
        VLOG(3) << "number of images in the buffer: " << data_.size();

        CHECK_LE(data_.back().stamp - data_.front().stamp, buffer_size_ns_);
    }

    bool CameraSimulator::imageCallback(
        const Image& img, Time time, const ImagePtr& camera_image
    ) {
        CHECK(camera_image);
        CHECK_EQ(camera_image->size(), img.size());

        buffer_->addImage(time, img);

        static const Time initial_time = time;
        if (time - initial_time < exposure_time_) {
            LOG_FIRST_N(WARNING, 1) << "The images do not cover a time span "
                                       "long enough to simulate the "
                                       "exposure time accurately.";
            return false;
        }

        // average all the images in the buffer to simulate motion blur
        camera_image->setTo(0);
        ze::real_t denom = 0.;
        for (const ImageBuffer::ImageData& img : buffer_->getRawBuffer()) {
            *camera_image +=
                ze::nanosecToMillisecTrunc(img.exposure_time) * img.image;
            denom += ze::nanosecToMillisecTrunc(img.exposure_time);
        }

        static uint frame_number = 0;

        std::stringstream ss;
        ss << output_folder << "/frames/frame_" << std::setfill('0')
           << std::setw(5) << frame_number++ << ".exr";
        std::string frame_path = ss.str();
        cv::imwrite(frame_path, *camera_image);

        // ze::openOutputFileStream(
        //     ze::joinPath(output_folder, "exposures.csv"),
        //     &exposures_file_
        // );

        exposures_file_.open(
            ze::joinPath(output_folder, "exposures.csv"),
            std::ios::app
        );
        exposures_file_ << time << "," << frame_number << ","
                        << buffer_->getExposureTime() << std::endl;
        exposures_file_.close();


        *camera_image /= denom;
        cv::Mat disp;
        camera_image->convertTo(disp, CV_8U, 255);

        return true;
    }

} // namespace event_camera_simulator
