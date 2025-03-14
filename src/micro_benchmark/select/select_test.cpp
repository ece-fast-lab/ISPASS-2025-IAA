//* [QPL_LOW_LEVEL_SELECT_EXAMPLE] */

#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <string>
#include <chrono>
#include <thread>

#include "qpl/qpl.h"

/**
 * @brief This example requires a command line argument to set the execution path. Valid values are `software_path`
 * and `hardware_path`.
 * In QPL, @ref qpl_path_software (`Software Path`) means that computations will be done with CPU.
 * Accelerator can be used instead of CPU. In this case, @ref qpl_path_hardware (`Hardware Path`) must be specified.
 * If there is no difference where calculations should be done, @ref qpl_path_auto (`Auto Path`) can be used to allow
 * the library to chose the path to execute. The Auto Path usage is not demonstrated by this example.
 *
 * @warning ---! Important !---
 * `Hardware Path` doesn't support all features declared for `Software Path`
 *
 */

/**
 * NOTE : Maximum transfer size per grouped_workqueues of IAA is 2097152(2MB)
 * If you want to put data larger than 2MB, you have to split the data into 2MB chunks.
 */
std::size_t chunk_size = 2097152;
// const std::size_t chunk_size = 1048576;
// const std::size_t chunk_size = 524288;
// const std::size_t chunk_size = 262144;
// const std::size_t chunk_size = 131072;
// const std::size_t chunk_size = 65536;
// const std::size_t chunk_size = 32768;
// const std::size_t chunk_size = 16384;
// const std::size_t chunk_size = 8192;
// const std::size_t chunk_size = 4096;
// const std::size_t chunk_size = 2048;
// const std::size_t chunk_size = 1024;
constexpr const uint32_t input_vector_width = 8;
constexpr const uint32_t boundary           = 71; //'G'

int parse_execution_path(int argc, char **argv, qpl_path_t *path_ptr, int extra_arg = 0) {
    // Get path from input argument
    if (extra_arg == 0) {
        if (argc < 2) {
            std::cout << "Missing the execution path as the first parameter. Use either hardware_path or software_path." << std::endl;
            return 1;
        }
    } else {
        if (argc < 4) {
            std::cout << "Missing the execution path as the first parameter and/or the dataset path as the second and third parameter." << std::endl;
            return 1;
        }
    }

    std::string path = argv[1];
    if (path == "hardware_path") {
        *path_ptr = qpl_path_hardware;
        std::cout << "The test will be run on the hardware path." << std::endl;
    } else if (path == "software_path") {
        *path_ptr = qpl_path_software;
        std::cout << "The test will be run on the software path." << std::endl;
    } else {
        std::cout << "Unrecognized value for parameter. Use hardware_path or software_path." << std::endl;
        return 1;
    }

    return 0;
}

void job_execution(qpl_job *job_ptr)
{
    qpl_status status = qpl_execute_job(job_ptr);
    if (status != QPL_STS_OK) {
        std::cout << "An error " << status << " acquired during job execution." << std::endl;
    }
}

int iaa_select(std::string src_data_file_path, std::string dest_data_file_path, qpl_path_t execution_path, uint32_t &iteration, const uint32_t queue_size)
{
    // Source and output containers
    std::vector<uint8_t> whole_src_vector;
    // std::vector<std::vector<uint8_t>> src_vector;
    std::vector<std::vector<uint8_t>> mask_after_scan;
    std::vector<std::vector<uint8_t>> dest_vector;
    double elapsed_time_sec = 0;

    std::cout << "[IAA Select]" << std::endl;
    
    // Opening source file
    std::cout << "Source file = " << src_data_file_path << std::endl;
    std::cout << "Loading source file... ";
    std::ifstream src_file;
    src_file.open(src_data_file_path, std::ifstream::in | std::ifstream::binary);
    if (!src_file) {
        std::cout << "File not found : " << src_data_file_path << std::endl;
        return 1;
    }
    std::cout << "Done" << std::endl;

    // Getting source file size
    src_file.seekg(0, std::ios::end);
    std::size_t src_file_size = static_cast<std::size_t>(src_file.tellg());
    src_file.seekg(0, std::ios::beg);

    // Job initialization
    std::vector<std::unique_ptr<uint8_t[]>> job_buffer;
    std::vector<qpl_job *>                  job;
    qpl_status                              status;
    uint32_t                                size = 0;

    // Allocation
    // src_vector.resize(queue_size);
    mask_after_scan.resize(queue_size);
    dest_vector.resize(queue_size);
    job_buffer.resize(queue_size);
    job.resize(queue_size);

    status = qpl_get_job_size(execution_path, &size);
    if (status != QPL_STS_OK) {
        std::cout << "An error " << status << " acquired during job size getting." << std::endl;
        return 1;
    }

    for (int i = 0; i < queue_size; ++i) {
        job_buffer[i] = std::make_unique<uint8_t[]>(size);
        job[i] = reinterpret_cast<qpl_job *>(job_buffer[i].get());
        status = qpl_init_job(execution_path, job[i]);
        if (status != QPL_STS_OK) {
            std::cout << "An error " << status << " acquired during job initializing." << std::endl;
            return 1;
        }
    }

    std::chrono::duration<int64_t, std::nano> elapsed_time_ns = std::chrono::nanoseconds::zero();
    std::size_t src_file_left = src_file_size;
    std::size_t vector_size = 0;
    iteration = 0;

    whole_src_vector.resize(src_file_size);

    // Load memory 
    src_file.read(reinterpret_cast<char *>(&whole_src_vector.front()), src_file_size);

    // Closing source file
    src_file.close();
    std::size_t current_idx = 0;

    std::chrono::duration<int64_t, std::nano> whole_elapsed_time_ns = std::chrono::nanoseconds::zero();
    auto whole_start = std::chrono::steady_clock::now();

    // Scan & Select
    while(src_file_left > 0) {
        int enqueue_cnt = 0;
        for (int i = 0; i < queue_size; ++i) {
            // Resizing source and destination vectors
            if (src_file_left <= chunk_size) {
                vector_size = src_file_left;
            } else {
                vector_size = chunk_size;
            }
            // src_vector[i].resize(vector_size);
            mask_after_scan[i].resize(vector_size);
            dest_vector[i].resize(vector_size);
            // Loading data from source file to source vector
            // src_file.read(reinterpret_cast<char *>(&src_vector[i].front()), vector_size);

            // Performing a scan operation
            job[i]->op                 = qpl_op_scan_eq;
            job[i]->level              = qpl_default_level;
            job[i]->next_in_ptr        = (uint8_t*)whole_src_vector.data() + current_idx;
            job[i]->next_out_ptr       = mask_after_scan[i].data();
            job[i]->available_in       = static_cast<uint32_t>(vector_size);
            job[i]->available_out      = static_cast<uint32_t>(vector_size);
            job[i]->src1_bit_width     = input_vector_width;
            job[i]->num_input_elements = static_cast<uint32_t>(vector_size);
            job[i]->out_bit_width      = qpl_ow_nom;
            job[i]->param_low          = boundary;

            status = qpl_execute_job(job[i]);
            if (status != QPL_STS_OK) {
                std::cout << "In Scan execute, An error " << status << " acquired during job execution." << std::endl;
                return 1;
            }
            const auto mask_length = job[i]->total_out;

            // Performing a select operation
            job[i]->op                 = qpl_op_select;
            job[i]->level              = qpl_default_level;
            job[i]->next_in_ptr        = (uint8_t*)whole_src_vector.data() + current_idx;
            job[i]->next_out_ptr       = dest_vector[i].data();
            job[i]->available_in       = static_cast<uint32_t>(vector_size);
            job[i]->available_out      = static_cast<uint32_t>(vector_size);
            job[i]->src1_bit_width     = input_vector_width;
            job[i]->num_input_elements = static_cast<uint32_t>(vector_size);
            job[i]->out_bit_width      = qpl_ow_nom;
            job[i]->next_src2_ptr      = mask_after_scan[i].data();
            job[i]->available_src2     = mask_length;
            job[i]->src2_bit_width     = 1;

            src_file_left -= vector_size;
            enqueue_cnt = i + 1;
            if (src_file_left == 0) { break; }
            current_idx += vector_size;
        }
        // Submit & Waiting
        if(execution_path == qpl_path_software) {
            std::vector<std::thread *> job_th;
            job_th.resize(enqueue_cnt);
            auto start = std::chrono::steady_clock::now();
            for (int i = 0; i < enqueue_cnt; ++i) { job_th[i] = new std::thread(job_execution, job[i]); }
            for (int i = 0; i < enqueue_cnt; ++i) { job_th[i]->join(); }
            auto end = std::chrono::steady_clock::now();
            elapsed_time_ns += end - start;
            for (int i = 0; i < enqueue_cnt; ++i) { delete job_th[i]; }
        } else {
            auto start = std::chrono::steady_clock::now();
            for (int i = 0; i < enqueue_cnt; ++i) {
                status = qpl_submit_job(job[i]);
                if (status != QPL_STS_OK) {
                    std::cout << "An error " << status << " acquired during job execution." << std::endl;
                    return 1;
                }
            }
            for (int i = 0; i < enqueue_cnt; ++i) {
                if (qpl_check_job(job[i]) == QPL_STS_OK) {
                    continue;
                }
                status = qpl_wait_job(job[i]);
                // std::cout << "Select: " << i << "status: " << qpl_check_job(job[i]) << std::endl;
                if (status != QPL_STS_OK) {
                    std::cout << "An error " << status << " acquired during job waiting." << std::endl;
                    return 1;
                }
            }
            auto end = std::chrono::steady_clock::now();
            elapsed_time_ns += end - start;
        }

        // for (int i = 0; i < enqueue_cnt; ++i) {
        //     // Opening destination file
        //     std::ofstream dest_file;
        //     dest_file.open(dest_data_file_path + "." + std::to_string(iteration + i), std::ofstream::out | std::ofstream::binary);
        //     if (!dest_file) {
        //         std::cout << "File not found : " << dest_data_file_path << std::endl;
        //         return 1;
        //     }

        //     // Writing selected data to destination file
        //     dest_file.write(reinterpret_cast<char *>(&dest_vector[i].front()), static_cast<std::size_t>(job[i]->total_out));

        //     // Closing destination file
        //     dest_file.close();
        // }

        iteration += enqueue_cnt;

        std::cout << '\r';
        std::cout << "Progress ... " << (src_file_size - src_file_left) << " / " << src_file_size << " Bytes" << std::flush;
    }

    // Closing source file
    // src_file.close();
    auto whole_end = std::chrono::steady_clock::now();

    whole_elapsed_time_ns += whole_end - whole_start;

    // Freeing resources
    for (int i = 0; i < queue_size; ++i) {
        status = qpl_fini_job(job[i]);
        if (status != QPL_STS_OK) {
            std::cout << "An error " << status << " acquired during job finalization." << std::endl;
            return 1;
        }
    }

    std::cout << std::endl;
    std::cout << "Select was performed successfully." << std::endl;
    std::cout << "Input size      = " << src_file_size << " Bytes" << std::endl;
    elapsed_time_sec = static_cast<double>(elapsed_time_ns.count()) / 1000 / 1000 / 1000;
    std::cout << "Elapsed Time = " << elapsed_time_ns.count() << " ns (" << elapsed_time_sec << " s)" << std::endl;
    double whole_elapsed_time_sec = static_cast<double>(whole_elapsed_time_ns.count()) / 1000 / 1000 / 1000;
    std::cout << "Whole elapsed Time = " << whole_elapsed_time_ns.count() << " ns (" << whole_elapsed_time_sec << " s)" << std::endl;
    std::cout << "Bandwidth       = " << static_cast<double>(src_file_size) / 1024 / 1024 / elapsed_time_sec << " MB/s" << std::endl;

    return 0;
}

auto main(int argc, char** argv) -> int {
    std::cout << std::endl;
    std::cout << "Intel(R) Query Processing Library version is " << qpl_get_library_version() << ".\n";

    // Default to Software Path
    qpl_path_t execution_path = qpl_path_software;

    // Get path from input argument
    int parse_ret = parse_execution_path(argc, argv, &execution_path, 1);
    if (parse_ret != 0) {
        return 1;
    }

    // File path
    const std::string SRC_DATA_FILE_PATH    = argv[2];
    const std::string DEST_DATA_FILE_PATH   = SRC_DATA_FILE_PATH + ".iaa.selected";
    
    const uint32_t queue_size = static_cast<uint32_t>(atoi(argv[3]));
    uint32_t iteration = 0;

    std::cout << "Queue Size = " << queue_size << std::endl;
    std::cout << std::endl;
    chunk_size = static_cast<size_t>(atoi(argv[4]));
    // Select
    if(iaa_select(SRC_DATA_FILE_PATH, DEST_DATA_FILE_PATH, execution_path, iteration, queue_size) != 0) {
        std::cout << "An error acquired during iaa_execution(select)" << std::endl;
        return 1;
    }

    return 0;
}

//* [QPL_LOW_LEVEL_SELECT_EXAMPLE] */
