#include "easy_pbr/LabelMngr.h"

//configuru
#define CONFIGURU_WITH_EIGEN 1
#define CONFIGURU_IMPLICIT_CONVERSIONS 1
#include <configuru.hpp>
using namespace configuru;

//my stuff
#include "string_utils.h"

//c++
#include <iostream>
#include <fstream>

//loguru
#define LOGURU_REPLACE_GLOG 1
#include <loguru.hpp>

using namespace radu::utils;

namespace easy_pbr{

LabelMngr::LabelMngr(const configuru::Config& config):
    m_nr_classes(-1){

    init_params(config);
}

LabelMngr::LabelMngr(const std::string labels_file, const std::string color_scheme_file, const std::string frequency_file, const int unlabeled_idx ){
    m_unlabeled_idx=unlabeled_idx;
    read_data(labels_file ,color_scheme_file, frequency_file);
}

//creates some random colors for X nr of classes and set unlabeled idx to color black
LabelMngr::LabelMngr(const int nr_classes, const int unlabeled_idx ){
    m_unlabeled_idx=unlabeled_idx;
    m_nr_classes=nr_classes;


    //init all the internal stuff
    for (int i=0; i<nr_classes; i++){
        std::string class_name="class_" + std::to_string(i);
        m_idx2label.push_back( class_name );
        m_label2idx[class_name]=i;
    }

    //colorscheme
    m_C_per_class.resize(nr_classes,3);
    m_C_per_class.setRandom(); //fill with random in range [-1,1]
    m_C_per_class=(m_C_per_class+ Eigen::MatrixXd::Constant(nr_classes,3, 1.0) )*0.5;
    if(unlabeled_idx<nr_classes){
        m_C_per_class.row(unlabeled_idx).setZero(); //unlabeled index is always black
    }

    //freq
    m_frequency_per_class.resize(nr_classes);
    m_frequency_per_class.setZero();

}

void LabelMngr::init_params(const Config& config ){

    std::string labels_file = (std::string)config["labels_file"];
    std::string color_scheme_file = (std::string)config["color_scheme_file"];
    std::string frequency_file = (std::string)config["frequency_file"];
    m_unlabeled_idx=config["unlabeled_idx"];
    VLOG(1) << "Read a unabeled idx of " << m_unlabeled_idx;

    read_data(labels_file ,color_scheme_file, frequency_file);

}

void LabelMngr::clear(){
    m_nr_classes=-1;
    // m_unlabeled_idx=-1; //do not change this because it will change what we have read from the config file
    m_idx2label.clear();
    m_label2idx.clear();
    m_C_per_class.resize(0,0);
    m_frequency_per_class.resize(0,0);
    m_idx_uncompacted2idx_compacted.clear();
    m_idx_compacted2idx_uncompacted.clear();
}

void LabelMngr::read_data(const std::string labels_file, const std::string color_scheme_file, const std::string frequency_file){
    //clear previous data
    clear();

    int nr_classes_read=0;
    std::ifstream labels_input(labels_file );
    if(!labels_input.is_open()){
        LOG(FATAL) << "Could not open labels file " << labels_file;
    }
    for( std::string line; getline( labels_input, line ); ){
        if(line.empty()){
            continue;
        }
        //skip lines that start with # because that's a comment
        if( (ltrim_copy(line)).at(0)!='#' && !line.empty() ){
            m_idx2label.push_back( trim_copy(line) );
            m_label2idx[line]=nr_classes_read;;
            // std::cout << "read leabel: " << line << " and inserted with idx" << nr_classes_read << '\n';
            nr_classes_read++;
        }
    }
    m_nr_classes=nr_classes_read;

    //read color scheme
    m_C_per_class.resize(m_nr_classes,3);
    m_C_per_class.setZero();
    int idx_insert=0;
    std::ifstream color_scheme_intput(color_scheme_file );
    if(!color_scheme_intput.is_open()){
        LOG(FATAL) << "Could not open color_scheme file " << color_scheme_file;
    }
    for( std::string line; getline( color_scheme_intput, line ); ){
        if(line.empty()){
            continue;
        }
        //skip lines that start with # because that's a comment
        if((ltrim_copy(line)).at(0)!='#' && !trim_copy(line).empty() ){

            // std::cout << "read color " << line << std::endl;

            //if we have more colors than necesarry don't add them
            if(idx_insert>=m_nr_classes){
                LOG(FATAL) << "Why do we have more colors than number of classes? Something is wrong. We have nr_classes: " << m_nr_classes << " but we are about to insert into idx " << idx_insert;
                break;
            }

            std::vector<std::string> rgb_vec= split(line,",");
            CHECK(rgb_vec.size()==3) << "We should have read 3 values for rgb but we read " << rgb_vec.size();
            m_C_per_class.row(idx_insert) << atoi( trim_copy(rgb_vec[0]).c_str()),
                                             atoi( trim_copy(rgb_vec[1]).c_str()),
                                             atoi( trim_copy(rgb_vec[2]).c_str()); //range [0,255]
            idx_insert++;

        }
    }
    m_C_per_class.array()/=255.0;

    // initialize_colormap_with_mapillary_colors(m_C_per_class);
    std::cout << "LabelMngr: nr of classes read " << m_nr_classes << '\n';

    // std::cout << "colors is \n" << m_C_per_class << '\n';


    //read frequencies of each class
    m_frequency_per_class.resize(m_nr_classes);
    m_frequency_per_class.setZero();
    int class_frequencies_read=0;
    std::ifstream frequency_input(frequency_file );
    if(!frequency_input.is_open()){
        LOG(FATAL) << "Could not open frequency file " << frequency_file;
    }
    for( std::string line; getline( frequency_input, line ); ){
        if(line.empty()){
            continue;
        }
        if(class_frequencies_read>=m_nr_classes){
            LOG(FATAL) << "Why do we have more frequencies than number of classes? Something is wrong. We have nr_classes: " << m_nr_classes << " but we are about to insert into idx " << class_frequencies_read;
            break;
        }

        //skip lines that start with # because that's a comment
        if( (ltrim_copy(line)).at(0)!='#' && !line.empty() ){
            m_frequency_per_class(class_frequencies_read) =  stof(trim_copy(line));
            class_frequencies_read++;
        }
    }
    // VLOG(1) << "CLass frequencies is " << m_frequency_per_class;
    CHECK(class_frequencies_read==m_nr_classes) << "We must have the same nr of classes as frequencies for each one. We have m_nr_classes " <<m_nr_classes << " and we have read nr of frequencies " << class_frequencies_read;



}

int LabelMngr::nr_classes(){
    CHECK(m_nr_classes!=-1) << "The label mngr has not been initialized yet. Please use LabelMngr.init() first.";
    return m_nr_classes;
}

// int LabelMngr::get_idx_background(){
//     return m_label2idx[m_background_label];
// }

int LabelMngr::get_idx_unlabeled(){
    return m_unlabeled_idx;
}

// int LabelMngr::get_idx_invalid(){
//     return m_nr_classes+1;
// }

std::string LabelMngr::idx2label(int idx){
    CHECK(m_nr_classes!=-1) << "The label mngr has not been initialized yet. Please use LabelMngr.init() first.";
    return m_idx2label[idx];
}
int LabelMngr::label2idx(std::string label){
    CHECK(m_nr_classes!=-1) << "The label mngr has not been initialized yet. Please use LabelMngr.init() first.";
    CHECK(m_label2idx.find(label)!=m_label2idx.end()) << "The label "<< label << " does not exist";
    return m_label2idx[label];
}

Eigen::Vector3d LabelMngr::color_for_label(const int idx){
    CHECK(m_nr_classes!=-1) << "The label mngr has not been initialized yet. Please use LabelMngr.init() first.";
    return m_C_per_class.row(idx).transpose();
}
void LabelMngr::set_color_for_label_with_idx(const int idx, Eigen::Vector3d color){
    CHECK(m_nr_classes!=-1) << "The label mngr has not been initialized yet. Please use LabelMngr.init() first.";
    m_C_per_class.row(idx)=color;
}
void LabelMngr::set_color_scheme(Eigen::MatrixXd& color_per_class){
    m_C_per_class=color_per_class;
    m_nr_classes=color_per_class.rows();
}

Eigen::MatrixXd LabelMngr::color_scheme(){
    CHECK(m_nr_classes!=-1) << "The label mngr has not been initialized yet. Please use LabelMngr.init() first.";
    return m_C_per_class;
}

Eigen::MatrixXf LabelMngr::color_scheme_for_gl(){
    CHECK(m_nr_classes!=-1) << "The label mngr has not been initialized yet. Please use LabelMngr.init() first.";
    Eigen::MatrixXf C_per_class_transpose_float=m_C_per_class.transpose().cast<float>();
    return C_per_class_transpose_float;
}
Eigen::VectorXf LabelMngr::class_frequencies(){
    CHECK(m_nr_classes!=-1) << "The label mngr has not been initialized yet. Please use LabelMngr.init() first.";
    return m_frequency_per_class;
}

void LabelMngr::compact(const std::string label_to_remove){
    CHECK(m_nr_classes!=-1) << "The label mngr has not been initialized yet. Please use LabelMngr.init() first.";
    CHECK(m_idx_uncompacted2idx_compacted.empty()) << "You already called compact() before, therefore the m_idx_uncompacted2idx_compacted was already created. If we were to overwrite it it would mean we would have no way to reindex into the original labels.";
    //removed a certain label but keeps the other ones. We have to recompute the m_label2idx and m_idx2label
    int new_nr_classes;
    std::vector<std::string> new_idx2label;
    std::unordered_map<std::string,int> new_label2idx;
    Eigen::MatrixXd new_C_per_class;
    Eigen::VectorXf new_frequency_per_class;


    //redo m_idx2label, m_label2idx, and m_nr_classes
    new_nr_classes=0;
    // m_idx_uncompacted2idx_compacted.resize(m_nr_classes, -1);
    m_idx_uncompacted2idx_compacted.resize(m_nr_classes, m_unlabeled_idx);
    for(size_t i=0; i<m_idx2label.size(); i++){
        std::string label=m_idx2label[i];
        if(label!=label_to_remove){
            new_idx2label.push_back(label);
            new_label2idx[label]=new_nr_classes;
            m_idx_uncompacted2idx_compacted[i]=new_nr_classes;
            m_idx_compacted2idx_uncompacted.push_back(i);
            new_nr_classes++;
        }
    }
    CHECK(m_nr_classes!=new_nr_classes) << "I expected that through compaction,  the nr of classes would decrease. However this was not the case. Maybe the label you wanted to remove was not found.";
    std::cout << "LabelMngr: nr of classes after compaction " << new_nr_classes << '\n';

    //reado m_C_per_class and frequencies
    new_C_per_class.resize(new_nr_classes,3);
    new_C_per_class.setZero();
    new_frequency_per_class.resize(new_nr_classes);
    new_frequency_per_class.setZero();
    int valid_class_idx=0;
    for(size_t i=0; i<m_idx2label.size(); i++){
        std::string label=m_idx2label[i];
        if(label!=label_to_remove){
            new_C_per_class.row(valid_class_idx)=m_C_per_class.row(i);
            new_frequency_per_class(valid_class_idx)=m_frequency_per_class(i);
            valid_class_idx++;
        }
    }


    //assign all the new things
    m_nr_classes=new_nr_classes;
    m_idx2label=new_idx2label;
    m_label2idx=new_label2idx;
    m_C_per_class=new_C_per_class;
    m_frequency_per_class=new_frequency_per_class;

}

//given a matrix of Nx1 of labels indices, reindex them to point into the compacted classes. This is useful after we used a label_mngr->compact to reindex the classes of a cloud
void LabelMngr::reindex_into_compacted_labels(Eigen::MatrixXi& labels_indices){
    CHECK(m_nr_classes!=-1) << "The label mngr has not been initialized yet. Please use LabelMngr.init() first.";
    CHECK(!m_idx_uncompacted2idx_compacted.empty()) << "This function can be used only after we used label_mngr->compact(). This will create a mapping between the uncompacted and compacted labels. At the moment the mapping is not created";

    bool found_idx_putside_of_range=false;
    int nr_indexes_outside_of_range=0;

    for(int i=0; i<labels_indices.rows(); i++){
        int initial_idx=labels_indices(i);
        if(initial_idx>(int)m_idx_uncompacted2idx_compacted.size()){
            found_idx_putside_of_range=true;
            nr_indexes_outside_of_range++;
            labels_indices(i)=m_unlabeled_idx;
        }else{
            int reindexed_idx=m_idx_uncompacted2idx_compacted[initial_idx];
            CHECK(reindexed_idx!=-1) << "Why is it reindexing into a -1? Something went wrong with the creation of the m_idx_uncompacted2idx_compacted. The error happened when trying to map the initial_label_idx of " << initial_idx <<" . The label_mngr has unlabeled_idx set to " << m_unlabeled_idx;
            labels_indices(i)=reindexed_idx;
        }
    }


    if(found_idx_putside_of_range){
        LOG(WARNING) << "Found " << nr_indexes_outside_of_range << "point witch a lable idx outside of the range of m_nr_classes so we set them to unlabeled";
    }

}

void LabelMngr::reindex_into_uncompacted_labels(Eigen::MatrixXi& labels_indices){
    CHECK(m_nr_classes!=-1) << "The label mngr has not been initialized yet. Please use LabelMngr.init() first.";
    CHECK(!m_idx_compacted2idx_uncompacted.empty()) << "This function can be used only after we used label_mngr->compact(). This will create a mapping between the uncompacted and compacted labels. At the moment the mapping is not created";

    for(int i=0; i<labels_indices.rows(); i++){
        int initial_idx=labels_indices(i);
        int reindexed_idx=m_idx_compacted2idx_uncompacted[initial_idx];
        CHECK(reindexed_idx!=-1) << "Why is it reindexing into a -1? Something went wrong with the creation of the m_idx_uncompacted2idx_compacted. The error happened when trying to map the initial_label_idx of " << initial_idx <<" . The label_mngr has unlabeled_idx set to " << m_unlabeled_idx;
        labels_indices(i)=reindexed_idx;
    }

}



cv::Mat LabelMngr::apply_color_map(cv::Mat classes){
    CHECK(m_nr_classes!=-1) << "The label mngr has not been initialized yet. Please use LabelMngr.init() first.";
    cv::Mat classes_colored(classes.rows, classes.cols, CV_8UC3, cv::Scalar(0, 0, 0));
    for (int i = 0; i < classes.rows; i++) {
        for (int j = 0; j < classes.cols; j++) {
            int label=(int)classes.at<unsigned char>(i,j);
            classes_colored.at<cv::Vec3b>(i,j)[0]=m_C_per_class(label,2)*255; //remember opencv is BGR so we put the B first
            classes_colored.at<cv::Vec3b>(i,j)[1]=m_C_per_class(label,1)*255; //remember opencv is BGR so we put the B first
            classes_colored.at<cv::Vec3b>(i,j)[2]=m_C_per_class(label,0)*255; //remember opencv is BGR so we put the B first
        }
    }
    return classes_colored;
}

// void LabelMngr::apply_color_map(MeshCore& mesh){
//     //read the L, write in C

// }


} //namespace easy_pbr
