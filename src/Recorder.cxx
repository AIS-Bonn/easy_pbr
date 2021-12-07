#include "easy_pbr/Recorder.h"

//c++
#include "easy_gl/UtilsGL.h"



//my stuff
#include "easy_pbr/Viewer.h"
#include "easy_pbr/Camera.h"
// #include "opencv_utils.h" //only for debugging
#define ENABLE_GL_PROFILING 1
#include "Profiler.h"

//boost
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

// #include <ros/ros.h>

namespace easy_pbr{

Recorder::Recorder(Viewer* view):
    m_is_recording(false),
    m_view(view),
    m_nr_images_recorded(0)
    // m_recording_path("./recordings/"),
    // m_snapshot_name("img.png")
{


    // //initialize the pbo used for reading the default framebuffer
    // m_pbo_storages_initialized.resize(m_nr_pbos,false);
    // m_pbo_ids.resize(m_nr_pbos);
    // m_full_paths.resize(m_nr_pbos);
    // glGenBuffers(m_nr_pbos, m_pbo_ids.data());
    // m_idx_pbo_write=0;
    // m_idx_pbo_read=1; //just one in front of the writing one so it will take one full loop of all pbos for it to catch up

    m_writer_threads.resize(8);
    m_threads_are_running=true;
    for(size_t i = 0; i < m_writer_threads.size(); i++){
        m_writer_threads[i]=std::thread( &Recorder::write_to_file_threaded, this);
    }

}

Recorder::~Recorder(){
    //wait for the threads to write all the rest of the images
    while(m_cv_mats_queue.size_approx()!=0){
        VLOG(1) << "waiting for recording to finish";
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));


    m_threads_are_running=false;
    for(size_t i = 0; i < m_writer_threads.size(); i++){
        m_writer_threads[i].join();
    }
}

bool Recorder::record(gl::Texture2D& tex, const std::string name, const std::string path){
    GL_C( tex.download_to_pbo() );

    if( m_cv_mats_queue.size_approx()>100)
        VLOG(1) << "Enqueued too many cv_mats approx mats in queue: " << m_cv_mats_queue.size_approx();

    if(tex.cur_pbo_download().storage_initialized() ){
        int cv_type=gl_internal_format2cv_type(tex.internal_format());
        cv::Mat cv_mat = cv::Mat::zeros(cv::Size(tex.cur_pbo_download().width(), tex.cur_pbo_download().height()), cv_type); //the size of the texture is not the same as the pbo we ae downloading from because the pbo is delayed a couple of frames so a resizing of texture takes a while to take effect
        // VLOG(1) <<"writing mat of type " << easy_pbr::utils::type2string(cv_mat.type());

        tex.download_from_oldest_pbo(cv_mat.data);

        // //increment the nr of times we have written a tex with this name and create the path where we will write the mat
        // auto got = m_times_written_for_tex.find (name);
        // int nr_times_written=0;
        // if ( got == m_times_written_for_tex.end() ){
        //     m_times_written_for_tex[name]=0;
        // }else{
        //     nr_times_written=m_times_written_for_tex[name];
        //     m_times_written_for_tex[name]++;
        // }

        MatWithFilePath mat_with_file;
        mat_with_file.cv_mat=cv_mat;
        mat_with_file.file_path= ( fs::path(path)/name ).string();
        m_cv_mats_queue.enqueue(mat_with_file);

        if (is_recording()){
            m_nr_images_recorded++;
        }


        return true;
    }else{
        return false;
    }

    if( m_cv_mats_queue.size_approx()>100){
        LOG(FATAL) << "Enqueued too many cv_mats and couldn't write all of them in time. Consider adding more threads for writing or slowing down the enqueueing";
    }

}

void Recorder::write_without_buffering(gl::Texture2D& tex, const std::string name, const std::string path){


    cv::Mat cv_mat;
    cv_mat=tex.download_to_cv_mat();

    MatWithFilePath mat_with_file;
    mat_with_file.cv_mat=cv_mat;
    mat_with_file.file_path= ( fs::path(path)/name ).string();
    m_cv_mats_queue.enqueue(mat_with_file);

    if( m_cv_mats_queue.size_approx()>100){
        LOG(FATAL) << "Enqueued too many cv_mats and couldn't write all of them in time. Consider adding more threads for writing or slowing down the enqueueing";
    }

}

bool Recorder::record(const std::string name, const std::string path){
    //TODO put the bool for record_gui in the Recorder and then recorder either the final_fbo_with or without gui
    bool recorded=record(m_view->m_final_fbo_no_gui.tex_with_name("color_with_transparency_gtex"), name, path);
    return recorded;
}

void Recorder::snapshot(const std::string name, const std::string path){
    //TODO put the bool for record_gui in the Recorder and then recorder either the final_fbo_with or without gui
    write_without_buffering(m_view->m_final_fbo_no_gui.tex_with_name("color_with_transparency_gtex"), name, path);
}

void Recorder::record_orbit(const std::string path){
    int nr_images=360;
    float angle_increment=360/nr_images; //each segment needs to have this many angles
    m_view->m_show_gui=false; //this function is usually called by the gui, if we do again a view->update, it will try to make another frame for the gui and that will fail
    for(int i=0; i<360; i++){
        m_view->m_camera->orbit_y(angle_increment);
        m_view->update();
        std::string snapshot_name=std::to_string(i)+".png";
        //record
        if (m_view->m_record_with_transparency){
            write_without_buffering(m_view->m_final_fbo_no_gui.tex_with_name("color_with_transparency_gtex"), snapshot_name, path);
        }else{
            write_without_buffering(m_view->m_final_fbo_no_gui.tex_with_name("color_without_transparency_gtex"), snapshot_name, path);
        }
    }
    m_view->m_show_gui=true;
}


// void Recorder::write_viewer_to_png(){

//     //make the dirs
//     fs::path root (std::string(PROJECT_SOURCE_DIR));
//     fs::path dir (m_results_path);
//     fs::path png_name (m_single_png_filename);
//     fs::path full_path = root/ dir / png_name;
//     fs::create_directories(root/dir);

//     int width  = std::round(m_view->m_viewport_size.x() );
//     int height = std::round(m_view->m_viewport_size.y() );
//     width*=m_magnification;
//     height*=m_magnification;
//     // VLOG(1) << "Viewer has size of viewport" << m_view->m_viewport_size.x()  << " " << m_view->m_viewport_size.y();

//     //create a framebuffer to hold the final image
//     m_framebuffer.set_size(width*m_view->m_subsample_factor, height*m_view->m_subsample_factor ); //established what will be the size of the textures attached to this framebuffer
//     m_framebuffer.sanity_check();
//     // VLOG(1) << "framebuffer has size of" << m_framebuffer.width()  << " " << m_framebuffer.height();

//     //clear
//     m_framebuffer.bind();
//     m_view->clear_framebuffers();


//     // Save old viewport
//     Eigen::Vector2f old_viewport = m_view->m_viewport_size;
//     m_view->m_viewport_size << width,height;
//     // Draw
//     m_view->draw(m_framebuffer.get_fbo_id());
//     glBindFramebuffer(GL_FRAMEBUFFER, 0);
//     // Restore viewport
//     m_view->m_viewport_size = old_viewport;

//     //save the color texture to opencv mat
//     cv::Mat final_img;
//     final_img=m_framebuffer.tex_with_name("color_gtex").download_to_cv_mat();

//     //flip in the y axis and also change rgb to bgr because opencv..
//     cv::Mat final_img_flipped;
//     cv::flip(final_img, final_img_flipped, 0);
//     cv::cvtColor(final_img_flipped, final_img_flipped, CV_BGRA2RGBA);
//     cv::imwrite(full_path.string(),final_img_flipped);


// }

// void Recorder::record_viewer(){

//     //make the dirs
//     fs::path root (std::string(PROJECT_SOURCE_DIR));
//     fs::path dir (m_results_path);
//     fs::path png_name (std::to_string(m_nr_frames_recorded)+".png");
//     fs::path full_path = root/ dir / png_name;
//     fs::create_directories(root/dir);
//     VLOG(1) << "Recorder writing into " << full_path;

//     //attempt 2 to make the reading faster
//     // std::cout << "----------------------------------------------------------" << std::endl;
//     // std::cout << "writing in pbo " << m_idx_pbo_write <<std::endl;
//     // std::cout << "reading from pbo " << m_idx_pbo_read << std::endl;
//     int width=m_view->m_viewport_size.x()*m_view->m_subsample_factor;
//     int height=m_view->m_viewport_size.y()*m_view->m_subsample_factor;
//     TIME_START("record_write_pbo");
//     GL_C( glBindBuffer(GL_PIXEL_PACK_BUFFER, m_pbo_ids[m_idx_pbo_write]) );
//     //if the pbo write is not initialized, initialize it
//     int size_bytes=width*height*4*1; //4 channels of 1 byte each
//     if(!m_pbo_storages_initialized[m_idx_pbo_write]){
//         GL_C (glBufferData(GL_PIXEL_PACK_BUFFER, size_bytes, NULL, GL_STATIC_READ) ); //allocate storage for pbo
//         m_pbo_storages_initialized[m_idx_pbo_write]=true;
//     }
//     //send the current framebuffer to a the pbo_write
//     GL_C( glReadPixels(0, 0, width, height, GL_BGRA, GL_UNSIGNED_BYTE, 0) );
//     GL_C( glBindBuffer(GL_PIXEL_PACK_BUFFER, 0 ) );
//     TIME_END("record_write_pbo");
//     m_full_paths[m_idx_pbo_write] = full_path.string();

//     //copy previous pbo from gpu to cpu
//     TIME_START("record_read_pbo");
//     if(m_pbo_storages_initialized[m_idx_pbo_read]){
//         std::cout << "actually reading" <<std::endl;
//         cv::Mat cv_mat = cv::Mat::zeros(cv::Size(width, height), CV_8UC4);
//         GL_C( glBindBuffer(GL_PIXEL_PACK_BUFFER, m_pbo_ids[m_idx_pbo_read]) );
//         void* data = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
//         if (data != NULL){
//             memcpy(cv_mat.data, data, size_bytes);
// 	    }
//         glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
//         GL_C( glBindBuffer(GL_PIXEL_PACK_BUFFER, 0 ) );

//         //attempt 4
//         MatWithFilePath mat_with_file;
//         mat_with_file.cv_mat=cv_mat;
//         mat_with_file.file_path=m_full_paths[m_idx_pbo_read];
//         m_cv_mats_queue.enqueue(mat_with_file);
//         m_nr_frames_recorded++;


//     }
//     TIME_END("record_read_pbo");


//     //update indices
//     m_idx_pbo_write = (m_idx_pbo_write + 1) % m_nr_pbos;
//     m_idx_pbo_read = (m_idx_pbo_read + 1) % m_nr_pbos;




// }

void Recorder::write_to_file_threaded(){


    while(m_threads_are_running){

        MatWithFilePath mat_with_file;
        bool found = m_cv_mats_queue.try_dequeue(mat_with_file);
        if(!found){
            std::this_thread::sleep_for(std::chrono::milliseconds(3));
            continue;
        }


        TIME_START("write_to_file");


        //create folder
        fs::path folder=fs::path(mat_with_file.file_path).parent_path();
        if (!fs::exists(folder)){
            fs::create_directories(folder);
        }

        cv::Mat cv_mat_flipped;
        cv::flip(mat_with_file.cv_mat, cv_mat_flipped, 0);
        if(cv_mat_flipped.channels()==4){
            cv::cvtColor(cv_mat_flipped, cv_mat_flipped, cv::COLOR_BGRA2RGBA);
        }else if(cv_mat_flipped.channels()==3){
            cv::cvtColor(cv_mat_flipped, cv_mat_flipped, cv::COLOR_BGR2RGB);
        }
        VLOG(1) << "writen image to " << mat_with_file.file_path;
        bool result;
        try{
            result=cv::imwrite(mat_with_file.file_path, cv_mat_flipped);
        }catch (const cv::Exception& ex){
            LOG(FATAL) << "Exception saving image " << ex.what();
        }
        CHECK(result) << "Something went wrong when writing image";
        TIME_END("write_to_file");

    }

}


bool Recorder::is_recording(){
    return m_is_recording;
}
void Recorder::start_recording(){
    m_is_recording=true;
}
void Recorder::stop_recording(){
    m_is_recording=false;
    m_nr_images_recorded=0;
}
void Recorder::pause_recording(){
    m_is_recording=false;
}
int Recorder::nr_images_recorded(){
    return m_nr_images_recorded;
}
bool Recorder::is_finished(){
    // MatWithFilePath mat_with_file;
    // bool found = m_cv_mats_queue.try_dequeue(mat_with_file);
    bool is_empty= m_cv_mats_queue.size_approx() == 0;

    return is_empty;

    // if (found){
    //     return
    // }

}


} //namespace easy_pbr
