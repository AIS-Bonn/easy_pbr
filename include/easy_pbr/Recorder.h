#pragma once

#include <memory>
#include <mutex>
#include <thread>
#include <memory>
#include "GBuffer.h"

// #include "surfel_renderer/utils/shared_queue.h"
#include "concurrentqueue.h"

class Viewer;


//stored a cv mat and the path where it should be written to disk
struct MatWithFilePath{
    cv::Mat cv_mat;
    std::string file_path;
};

class Recorder: public std::enable_shared_from_this<Recorder>
{
public:
    Recorder();
    Recorder(Viewer* view);
    ~Recorder();
    // Recorder(const std::shared_ptr<Viewer> view);
    void write_viewer_to_png();
    void record_viewer(); //is called automatically by update() if the m_is_recording is set to true but sometimes I want to call it explicitly from python and record exatly when I want
    void update();
    void reset(); //set the m_nr_frames_recorder to zero so that we can start recording again


    //objects
    Viewer* m_view;

    //params 
    // char m_results_path[256] = "./results_movies/";
    // char m_single_png_filename[64] = "img.png";
    std::string m_results_path;
    std::string m_single_png_filename;
    float m_magnification;
    bool m_is_recording;

private:
    void write_to_file_threaded();


    gl::GBuffer m_framebuffer; //framebuffer in which we will draw, then we download it into a opencv mat in order to save it to disk
    int m_nr_frames_recorded;

    int m_nr_pbos;
    std::vector<GLuint> m_pbo_ids;
    std::vector<std::string> m_full_paths; //each time we write into a pbo, we also store the full path where it would be written to the hard disk. This is so that we can use reset() at any time
    std::vector<bool> m_pbo_storages_initialized;  
    int m_idx_pbo_write;
    int m_idx_pbo_read;


    //cv mats are buffered here and they await for the thread that writes them to file
    moodycamel::ConcurrentQueue<MatWithFilePath> m_cv_mats_queue;

    std::vector<std::thread> m_writer_threads; 
    // std::vector<cv::Mat> m_cv_mats_buffer;
    // int m_idx_last_mat_added;
    // int m_idx_mat_to_write;
    // bool m_need_to_write;
    // std::mutex m_mutex_update_last_added;

};