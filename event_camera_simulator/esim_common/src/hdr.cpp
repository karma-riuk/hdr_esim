#include "esim/common/hdr.hpp"

DEFINE_string(
    hdr_output_folder,
    "/home/arno/sim_ws/out",
    "Folder in which to output the events."
);

std::string hdr_output_folder = FLAGS_hdr_output_folder;
