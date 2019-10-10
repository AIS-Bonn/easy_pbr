#include "easy_pbr/Recorder.h"

//c++

//my stuff
#include "easy_pbr/Viewer.h"
#define ENABLE_GL_PROFILING 1
#include "Profiler.h"

//boost
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

// #include <ros/ros.h>


Recorder::Recorder():
    m_magnification(1.0),
    m_nr_frames_recorded(0),
    m_is_recording(false),
    m_nr_pbos(5),
    m_results_path("./results_movies/"),
    m_single_png_filename("img.png")
    // m_cv_mats_buffer(NUM_IMGS_BUFFERED),
    // m_idx_last_mat_added(0),
    // m_idx_mat_to_write(0),
    // m_need_to_write(false)
{

    m_framebuffer.add_texture("color_gtex", GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
    m_framebuffer.add_depth("depth_gtex");
    
    //initialize the pbo used for reading the default framebuffer
    m_pbo_storages_initialized.resize(m_nr_pbos,false);
    m_pbo_ids.resize(m_nr_pbos);
    m_full_paths.resize(m_nr_pbos);
    glGenBuffers(m_nr_pbos, m_pbo_ids.data());
    m_idx_pbo_write=0;
    m_idx_pbo_read=1; //just one in front of the writing one so it will take one full loop of all pbos for it to catch up

    m_writer_threads.resize(4);
    m_threads_are_running=true;
    for(size_t i = 0; i < m_writer_threads.size(); i++){
        m_writer_threads[i]=std::thread( &Recorder::write_to_file_threaded, this);
    }
    
    // m_writer_thread=std::thread( &Recorder::write_to_file_threaded, this);

}

Recorder::Recorder(Viewer* view):
    Recorder() // chain the default constructor
{
    m_view=view;
}

Recorder::~Recorder(){
    m_threads_are_running=false;
    for(size_t i = 0; i < m_writer_threads.size(); i++){
        m_writer_threads[i].join();
    }
    // m_writer_thread.join();
}
    


void Recorder::update(){
    if(m_is_recording){
        record_viewer();
    }
}

void Recorder::reset(){
    m_nr_frames_recorded=0;
    //this may clobber data from previous pbos in case that we didn't read from them, so that few frames of an animation will be lost...
    // std::fill( m_pbo_storages_initialized.begin(), m_pbo_storages_initialized.end(), false );
}

void Recorder::write_viewer_to_png(){

    //make the dirs 
    fs::path root (std::string(CMAKE_SOURCE_DIR));
    fs::path dir (m_results_path);
    fs::path png_name (m_single_png_filename);
    fs::path full_path = root/ dir / png_name;
    fs::create_directories(root/dir);

    int width  = std::round(m_view->m_viewport_size.x() );
    int height = std::round(m_view->m_viewport_size.y() );
    width*=m_magnification;
    height*=m_magnification;
    VLOG(1) << "Viewer has size of viewport" << m_view->m_viewport_size.x()  << " " << m_view->m_viewport_size.y();

    //create a framebuffer to hold the final image
    m_framebuffer.set_size(width*m_view->m_subsample_factor, height*m_view->m_subsample_factor ); //established what will be the size of the textures attached to this framebuffer
    m_framebuffer.sanity_check();
    VLOG(1) << "framebuffer has size of" << m_framebuffer.width()  << " " << m_framebuffer.height();

    //clear
    m_framebuffer.bind();
    m_view->clear_framebuffers();


    // Save old viewport
    Eigen::Vector2f old_viewport = m_view->m_viewport_size;
    m_view->m_viewport_size << width,height;
    // Draw
    m_view->draw(m_framebuffer.get_fbo_id());
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // Restore viewport
    m_view->m_viewport_size = old_viewport;

    //save the color texture to opencv mat
    cv::Mat final_img;
    final_img=m_framebuffer.tex_with_name("color_gtex").download_to_cv_mat();

    //flip in the y axis and also change rgb to bgr because opencv..
    cv::Mat final_img_flipped;
    cv::flip(final_img, final_img_flipped, 0);
    cv::cvtColor(final_img_flipped, final_img_flipped, CV_BGRA2RGBA);
    cv::imwrite(full_path.string(),final_img_flipped);


}

void Recorder::record_viewer(){

    //make the dirs 
    fs::path root (std::string(CMAKE_SOURCE_DIR));
    fs::path dir (m_results_path);
    fs::path png_name (std::to_string(m_nr_frames_recorded)+".png");
    fs::path full_path = root/ dir / png_name;
    fs::create_directories(root/dir);
    VLOG(1) << "Recorder writing into " << full_path;

    // //read the pixels directly from the default framebuffer
    // int width=m_view->m_viewport_size.x();
    // int height=m_view->m_viewport_size.y();
    // cv::Mat cv_mat = cv::Mat::zeros(cv::Size(width, height), CV_8UC3);
    // glReadPixels(0, 0, width, height, GL_BGR, GL_UNSIGNED_BYTE, cv_mat.data);


    //attempt 2 to make the reading faster
    std::cout << "----------------------------------------------------------" << std::endl;
    std::cout << "writing in pbo " << m_idx_pbo_write <<std::endl;
    std::cout << "reading from pbo " << m_idx_pbo_read << std::endl;
    int width=m_view->m_viewport_size.x()*m_view->m_subsample_factor;
    int height=m_view->m_viewport_size.y()*m_view->m_subsample_factor;
    TIME_START("record_write_pbo");
    GL_C( glBindBuffer(GL_PIXEL_PACK_BUFFER, m_pbo_ids[m_idx_pbo_write]) );
    //if the pbo write is not initialized, initialize it 
    int size_bytes=width*height*4*1; //4 channels of 1 byte each
    if(!m_pbo_storages_initialized[m_idx_pbo_write]){
        GL_C (glBufferData(GL_PIXEL_PACK_BUFFER, size_bytes, NULL, GL_STATIC_READ) ); //allocate storage for pbo
        m_pbo_storages_initialized[m_idx_pbo_write]=true;
    }
    //send the current framebuffer to a the pbo_write
    GL_C( glReadPixels(0, 0, width, height, GL_BGRA, GL_UNSIGNED_BYTE, 0) );
    GL_C( glBindBuffer(GL_PIXEL_PACK_BUFFER, 0 ) );
    TIME_END("record_write_pbo");
    m_full_paths[m_idx_pbo_write] = full_path.string();

    //copy previous pbo from gpu to cpu
    TIME_START("record_read_pbo");
    if(m_pbo_storages_initialized[m_idx_pbo_read]){
        std::cout << "actually reading" <<std::endl;
        cv::Mat cv_mat = cv::Mat::zeros(cv::Size(width, height), CV_8UC4);
        GL_C( glBindBuffer(GL_PIXEL_PACK_BUFFER, m_pbo_ids[m_idx_pbo_read]) );
        void* data = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
        if (data != NULL){
            memcpy(cv_mat.data, data, size_bytes);
	    }
        glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
        GL_C( glBindBuffer(GL_PIXEL_PACK_BUFFER, 0 ) );
        // cv_mat.data=(uchar*)data; //unsafe but it's faster than doing a full copy

        // write to file        
        // TIME_START("write_to_file");
        // cv::Mat cv_mat_flipped;
        // cv::flip(cv_mat, cv_mat_flipped, 0);
        // cv::imwrite(full_path.string(),cv_mat_flipped);
        // cv::imwrite(full_path.string(),cv_mat);
        // std::thread t1( &Recorder::write_to_file_threaded, this, cv_mat, full_path.string());


        // m_cv_mats_buffer[m_idx_mat_to_write]=cv_mat;
        // m_mutex_update_last_added.lock();
        // m_idx_last_mat_added=m_idx_mat_to_write;
        // m_mutex_update_last_added.unlock();
        // m_need_to_write=true;
        // m_idx_mat_to_write=(m_idx_mat_to_write+1) % NUM_IMGS_BUFFERED;

        //attempt 3
        // m_cv_mats_queue.push(cv_mat);

        //attempt 4
        MatWithFilePath mat_with_file;
        mat_with_file.cv_mat=cv_mat;
        mat_with_file.file_path=m_full_paths[m_idx_pbo_read];
        m_cv_mats_queue.enqueue(mat_with_file);
        m_nr_frames_recorded++;


        // TIME_END("write_to_file");
        // glUnmapBuffer(GL_PIXEL_PACK_BUFFER);


    }
    TIME_END("record_read_pbo");


    //update indices
    m_idx_pbo_write = (m_idx_pbo_write + 1) % m_nr_pbos;
    m_idx_pbo_read = (m_idx_pbo_read + 1) % m_nr_pbos;


   

}

void Recorder::write_to_file_threaded(){


    std::cout << "Thread for writing to file" << std::endl;

    // while(ros::ok()){
    while(m_threads_are_running){
        // if(m_need_to_write){
        //     m_need_to_write=false;
        // }else{
        //     std::this_thread::sleep_for(std::chrono::milliseconds(3));
        //     continue;
        // }

        // //make the dirs 
        // fs::path root (std::string(CMAKE_SOURCE_DIR));
        // fs::path dir (m_results_path);
        // fs::path png_name (std::to_string(m_frames_recorded)+".png");
        // fs::path full_path = root/ dir / png_name;
        // fs::create_directory(root/dir);

        // if(m_cv_mats_queue.size()==0){
        //     m_frames_recorded=0; // reset the counter to indicate that we start a new sequence
        // }


        // std::cout << "????????????writing============" << std::endl;
        // int local_idx_last_mat_added; //on this thread we store a local copy of the index of the img we need to write so the main thread can keep updating it if necessary
        // m_mutex_update_last_added.lock();
        // local_idx_last_mat_added=m_idx_last_mat_added;
        // m_mutex_update_last_added.unlock();

        // cv::Mat cv_mat;
        // m_cv_mats_queue.wait_and_pop(cv_mat);


        //attempt3 with moodycamel 
        MatWithFilePath mat_with_file;
        bool found = m_cv_mats_queue.try_dequeue(mat_with_file);
        if(!found){
            std::this_thread::sleep_for(std::chrono::milliseconds(3));
            continue;
        }



        // TIME_START("write_to_file");
        cv::Mat cv_mat_flipped;
        // cv::flip(m_cv_mats_buffer[local_idx_last_mat_added], cv_mat_flipped, 0);
        cv::flip(mat_with_file.cv_mat, cv_mat_flipped, 0);
        std::cout << "????????????writing============ to " << mat_with_file.file_path << std::endl;
        cv::imwrite(mat_with_file.file_path, cv_mat_flipped);
        // TIME_END("write_to_file");

        // m_frames_recorded++;
    }
   



}


