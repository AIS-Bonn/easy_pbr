#pragma once

#include <memory>
#include <mutex>
#include <thread>
#include <memory>
#include <unordered_map>
#include "easy_gl/GBuffer.h"

#include "concurrentqueue.h"

namespace easy_pbr{

class Viewer;


//stored a cv mat and the path where it should be written to disk
struct MatWithFilePath{
    cv::Mat cv_mat;
    std::string file_path;
};

class Recorder: public std::enable_shared_from_this<Recorder>
{
public:
    Recorder(Viewer* view);
    ~Recorder();
    bool record(gl::Texture2D& tex, const std::string name,  const std::string path); //downloads the tex into a pbo and downlaod from the previous pbo into a cv which is queued for writing
    void write_without_buffering(gl::Texture2D& tex, const std::string name,  const std::string path); //writes the texture directly, without PBO buffering. useful for taking screenshots
    bool record(const std::string name,  const std::string path);
    void snapshot(const std::string name,  const std::string path);
    void record_orbit( const std::string path); //makes a 360 orbit around the object and records the X images of it
    // void write_viewer_to_png();
    // void record_viewer(); //is called automatically by update() if the m_is_recording is set to true but sometimes I want to call it explicitly from python and record exatly when I want
    // void update();
    // void reset(); //set the m_nr_frames_recorder to zero so that we can start recording again

    bool is_recording();
    void start_recording();
    void pause_recording();
    void stop_recording();
    int nr_images_recorded();
    bool is_finished(); //returns true when the queue is empty and we finished writing everything


    //objects
    Viewer* m_view;
    // std::shared_ptr<Viewer> m_view;

    //params
    // std::string m_recording_path;
    // std::string m_snapshot_name;

private:
    void write_to_file_threaded();


    // gl::GBuffer m_framebuffer; //framebuffer in which we will draw, then we download it into a opencv mat in order to save it to disk
    // int m_nr_frames_recorded;

    // int m_nr_pbos;
    // std::vector<GLuint> m_pbo_ids;
    // std::vector<std::string> m_full_paths; //each time we write into a pbo, we also store the full path where it would be written to the hard disk. This is so that we can use reset() at any time
    // std::vector<bool> m_pbo_storages_initialized;
    // int m_idx_pbo_write;
    // int m_idx_pbo_read;


    // //cv mats are buffered here and they await for the thread that writes them to file
    moodycamel::ConcurrentQueue<MatWithFilePath> m_cv_mats_queue;
    // std::unordered_map<std::string, int> m_times_written_for_tex; //how many times we have written a texture with a certain name
    std::vector<std::thread> m_writer_threads;
    bool m_threads_are_running;

    bool m_is_recording;
    int m_nr_images_recorded;
};


} //namespace easy_pbr
