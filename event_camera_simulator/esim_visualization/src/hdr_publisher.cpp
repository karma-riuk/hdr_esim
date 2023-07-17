#include "esim/common/types.hpp"
#include "kindr/minimal/rotation-quaternion.h"

#include <esim/common/utils.hpp>
#include <esim/visualization/hdr_publisher.hpp>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <iomanip>
#include <opencv2/highgui/highgui.hpp>
#include <sys/stat.h>
#include <ze/common/file_utils.hpp>
#include <ze/common/path_utils.hpp>
#include <ze/common/time_conversions.hpp>

DEFINE_string(
    hdr_output_folder,
    "/home/arno/sim_ws/out",
    "Folder in which to output the events."
);

namespace event_camera_simulator {

    /**
     * This publisher was designed with the purpose of generating simulation
     * data with ground truth labels, for the task of optic flow estimation.
     *
     * It assumes that it will receive a relatively small sequence of events
     * (corresponding, for example, to all the events in between two frames),
     * and will write all the events to disk in its destructor, in three forms:
     *   - an "events.txt" file that contains all the events in "t x y pol"
     * format (one event per line)
     *   - an "event_count.png" image that whose first two channels contain the
     * counts of the positive (resp. negative) event counts at each pixel
     *   - two "timestamps images" in which each pixel contains the timestamp at
     * the last event that fell on the pixel. (since the timestamp is a floating
     * point value, it is split in 3 8-bit values so that the timestamp images
     *      can be saved in a single 3-channel image).
     */

    HdrPublisher::HdrPublisher(const std::string& output_folder)
        : output_folder_(output_folder) {
        if (!ze::isDir(output_folder_))
            mkdir(output_folder_.c_str(), 0777);
        if (!ze::isDir(output_folder_ + "/frames"))
            mkdir((output_folder_ + "/frames").c_str(), 0777);

        ze::openOutputFileStream(
            ze::joinPath(output_folder, "exposures.csv"),
            &exposures_file_
        );

        exposures_file_ << "timestamp,frame_number,exposure" << std::endl;
        exposures_file_.close();

        ze::openOutputFileStream(
            ze::joinPath(output_folder, "poses.csv"),
            &poses_file_
        );
        // Set the headers of the poses csv file
        poses_file_ << "timestamp,";
        poses_file_ << "px,";
        poses_file_ << "py,";
        poses_file_ << "pz,";
        poses_file_ << "qx,";
        poses_file_ << "qy,";
        poses_file_ << "qz,";
        poses_file_ << "qw";
        poses_file_ << std::endl;

        ze::openOutputFileStream(
            ze::joinPath(output_folder, "events.csv"),
            &events_file_
        );
        // Set the headers of the events csv file
        events_file_ << "timestamp,";
        events_file_ << "polarity,";
        events_file_ << "x,";
        events_file_ << "y";
        events_file_ << std::endl;
    }

    Publisher::Ptr HdrPublisher::createFromGflags() {
        if (FLAGS_hdr_output_folder == "") {
            LOG(WARNING) << "Empty output folder string: will not write "
                            "hdr files";
            return nullptr;
        }

        return std::make_shared<HdrPublisher>(FLAGS_hdr_output_folder);
    }

    HdrPublisher::~HdrPublisher() {
        events_file_.close();
        poses_file_.close();
    }

    void HdrPublisher::poseCallback(
        const Transformation& T_W_B, const TransformationVector& T_W_Cs, Time t
    ) {
        Vector3 p = T_W_B.getPosition();
        poses_file_ << t << "," << p.x() << "," << p.y() << "," << p.z() << ",";

        auto rotation = T_W_B.getRotation();
        poses_file_ << rotation.x() << "," << rotation.y() << ","
                    << rotation.z() << "," << rotation.w() << std::endl;
    }

    void HdrPublisher::imageCallback(const ImagePtrVector& images, Time t) {
        CHECK_EQ(images.size(), 1);
        static uint frame_number = 0;
        std::stringstream ss;
        ss << output_folder_ << "/frames/frame_" << std::setfill('0')
           << std::setw(5) << frame_number++ << ".exr";
        std::string frame_path = ss.str();

        cv::imwrite(frame_path, *images[0]);
    }

    void HdrPublisher::eventsCallback(const EventsVector& events) {
        CHECK_EQ(events.size(), 1);

        // Simply aggregate the events into the events_ buffer.
        // At the destruction of this object, everything will be saved to disk.
        for (const Event& e : events[0]) {
            events_file_ << e.t << "," << (e.pol ? 1 : -1) << "," << e.x << ","
                         << e.y << std::endl;
        }
    }

} // namespace event_camera_simulator
