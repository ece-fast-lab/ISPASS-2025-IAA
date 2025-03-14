//* [QPL_LOW_LEVEL_COMPRESSION_EXAMPLE] */

#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <string>
#include <chrono>
#include <thread>
#include <filesystem>
#include <regex>
#include <lz4.h>
#include <zlib.h>
#include <iomanip> 

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
const std::size_t chunk_size = 2097152;
constexpr const uint32_t input_vector_width     = 32;
constexpr const uint32_t shipdate_lower_boundary         = 757382400; // tpch q6 '1994-01-01' <=
constexpr const uint32_t shipdate_upper_boundary         = 788918400 - 1; // tpch q6 < '1995-01-01'
constexpr const uint32_t discount_lower_boundary         = 1028443340; // tpch q6 0.05 <=
constexpr const uint32_t discount_upper_boundary         = 1032805417; // tpch q6 <= 0.07
constexpr const uint32_t quantity_lower_boundary         = 0; // tpch q6 
constexpr const uint32_t quantity_upper_boundary         = 23; // tpch q6 < 24
// constexpr const uint32_t lower_boundary         = 757382400; 
// constexpr const uint32_t upper_boundary         = 788918400 - 1; 

void getFileSize(const std::string& filePath, int* fileSize) {
    // Open the file in binary mode, with the file pointer at the end
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (file) {
        // Get the file size using tellg() and save it to the provided int pointer
        *fileSize = static_cast<int>(file.tellg());
        file.close();
    } else {
        // If file cannot be opened, set fileSize to -1 to indicate an error
        *fileSize = -1;
        std::cerr << "Error: Unable to open file: " << filePath << std::endl;
    }
}

void printInts(const std::vector<uint8_t>& decompressed_result_vector, int idx) {
    // Ensure that the vector is large enough to contain at least 10 ints
    if (decompressed_result_vector.size() < 10 * sizeof(int32_t)) {
        std::cerr << "Error: The buffer is too small to contain 10 integers." << std::endl;
        return;
    }

    // Print the first 10 32-bit integers
    // Extract 4 bytes and combine them into an int32_t
    int32_t value = (decompressed_result_vector[idx * 4 + 0] << 0)  |  // Least significant byte
                    (decompressed_result_vector[idx * 4 + 1] << 8)  |
                    (decompressed_result_vector[idx * 4 + 2] << 16) |
                    (decompressed_result_vector[idx * 4 + 3] << 24);  // Most significant byte

    // Print the value
    // std::cout << value << std::endl;
}

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

double iaa_decompress_scan(std::string src_data_file_path, uint32_t iteration, const uint32_t queue_size, std::vector<qpl_job *>& job, std::vector<std::vector<uint8_t>>& mask, const uint32_t lower_boundary, const uint32_t upper_boundary, int* input_file_size, std::vector<uint32_t>& mask_length)
{
    qpl_path_t execution_path = qpl_path_hardware;
    std::vector<std::vector<uint8_t>> src_vector;
    qpl_status                              status;
    src_vector.resize(queue_size);
    std::chrono::duration<int64_t, std::nano> elapsed_time_ns = std::chrono::nanoseconds::zero();
    for(uint32_t file_id = 0; file_id < iteration;) {
        int enqueue_cnt = 0;
        for (int i = 0; i < queue_size; ++i) {
            std::ifstream src_file;
            src_file.open(src_data_file_path + "." + std::to_string(file_id), std::ifstream::in | std::ifstream::binary);
            if (!src_file) {
                std::cout << "File not found : " << src_data_file_path + "." + std::to_string(file_id) << std::endl;
                return -1;
            }
            src_file.seekg(0, std::ios::end);
            std::size_t src_file_size = static_cast<std::size_t>(src_file.tellg());
            src_file.seekg(0, std::ios::beg);
            src_vector[i].resize(src_file_size);
            src_file.read(reinterpret_cast<char *>(&src_vector[i].front()), src_file_size);
            src_file.close();

            job[i]->op             = qpl_op_scan_range;
            job[i]->next_in_ptr    = src_vector[i].data();
            job[i]->next_out_ptr   = mask[file_id].data();
            job[i]->available_in   = src_file_size;
            job[i]->available_out  = chunk_size;
            job[i]->src1_bit_width     = input_vector_width;
            job[i]->out_bit_width      = qpl_ow_nom;
            job[i]->param_low          = lower_boundary;
            job[i]->param_high         = upper_boundary;
            int num_input_elements = chunk_size;
            if (file_id == iteration - 1) {
                num_input_elements = *input_file_size - chunk_size * (iteration - 1);
            }
            job[i]->num_input_elements = static_cast<uint32_t>(num_input_elements)/ (input_vector_width / 8);
            job[i]->flags          = QPL_FLAG_FIRST | QPL_FLAG_LAST | QPL_FLAG_DECOMPRESS_ENABLE;
            enqueue_cnt = i + 1;
            ++file_id;
            if (file_id >= iteration) { break; }
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
                    std::cout << "An error " << status << " acquired during job submission." << std::endl;
                    return -1;
                }
            }
            for (int i = 0; i < enqueue_cnt; ++i) {
                // auto start_t = std::chrono::steady_clock::now();
                status = qpl_wait_job(job[i]);
                if (status != QPL_STS_OK) {
                    std::cout << "An error " << status << " acquired during job waiting." << std::endl;
                    return -1;
                }
            }
            auto end = std::chrono::steady_clock::now();
            elapsed_time_ns += end - start;
            for (int i = 0; i < enqueue_cnt; ++i) {
                mask_length[file_id - enqueue_cnt + i] = job[i]->total_out;
            }
        }
    }
    double elapsed_time_sec = static_cast<double>(elapsed_time_ns.count()) / 1000 / 1000 / 1000;
    return elapsed_time_sec;
}

double iaa_decompress_select(std::string src_data_file_path, uint32_t iteration, const uint32_t queue_size, std::vector<qpl_job *>& job, std::vector<std::vector<uint8_t>>& dest_vector, int* input_file_size, std::vector<std::vector<uint8_t>> bitmap, uint32_t bit_width, std::vector<uint32_t> mask_length, std::vector<uint32_t>& total_out)
{
    qpl_path_t execution_path = qpl_path_hardware;
    std::vector<std::vector<uint8_t>> src_vector;
    qpl_status                              status;
    src_vector.resize(queue_size);
    std::chrono::duration<int64_t, std::nano> elapsed_time_ns = std::chrono::nanoseconds::zero();
    for(uint32_t file_id = 0; file_id < iteration;) {
        int enqueue_cnt = 0;
        for (int i = 0; i < queue_size; ++i) {
            std::ifstream src_file;
            src_file.open(src_data_file_path + "." + std::to_string(file_id), std::ifstream::in | std::ifstream::binary);
            if (!src_file) {
                std::cout << "File not found : " << src_data_file_path + "." + std::to_string(file_id) << std::endl;
                return -1;
            }
            src_file.seekg(0, std::ios::end);
            std::size_t src_file_size = static_cast<std::size_t>(src_file.tellg());
            src_file.seekg(0, std::ios::beg);
            src_vector[i].resize(src_file_size);
            src_file.read(reinterpret_cast<char *>(&src_vector[i].front()), src_file_size);
            src_file.close();
            job[i]->op             = qpl_op_select;
            job[i]->next_in_ptr    = src_vector[i].data();
            job[i]->next_out_ptr   = dest_vector[file_id].data();
            job[i]->available_in   = src_file_size;
            job[i]->available_out  = chunk_size;
            job[i]->src1_bit_width     = input_vector_width;
            int num_input_elements = chunk_size;
            if (file_id == iteration - 1) {
                num_input_elements = *input_file_size - chunk_size * (iteration - 1);
            }
            job[i]->num_input_elements = static_cast<uint32_t>(num_input_elements) / (bit_width / 8);
            job[i]->next_src2_ptr      = bitmap[file_id].data();
            job[i]->available_src2     = mask_length[file_id];
            job[i]->src2_bit_width     = 1;
            job[i]->flags          = QPL_FLAG_FIRST | QPL_FLAG_LAST | QPL_FLAG_DECOMPRESS_ENABLE;
            enqueue_cnt = i + 1;
            ++file_id;
            if (file_id >= iteration) { break; }
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
                    std::cout << "An error " << status << " acquired during job submission." << std::endl;
                    return -1;
                }
            }
            for (int i = 0; i < enqueue_cnt; ++i) {
                // auto start_t = std::chrono::steady_clock::now();
                status = qpl_wait_job(job[i]);
                if (status != QPL_STS_OK) {
                    std::cout << "An error " << status << " acquired during job waiting." << std::endl;
                    return -1;
                }
            }
            auto end = std::chrono::steady_clock::now();
            elapsed_time_ns += end - start;
            for (int i = 0; i < enqueue_cnt; i++) {
                total_out[file_id - enqueue_cnt + i] = job[i]->total_out;
            }
        }
    }
    double elapsed_time_sec = static_cast<double>(elapsed_time_ns.count()) / 1000 / 1000 / 1000;
    return elapsed_time_sec;
}

double iaa_scan(std::vector<qpl_job *>& job, std::size_t scan_left, int& enqueue_cnt, const uint32_t queue_size, std::vector<std::vector<uint8_t>>& mask, std::size_t& source_idx, std::vector<uint8_t> decompressed_result_vector, const uint32_t lower_boundary, const uint32_t upper_boundary)
{
    uint32_t num_input = 0;
    qpl_status status;
    for (int i = 0; i < queue_size; ++i) {
        num_input = scan_left < chunk_size ? scan_left : chunk_size;
        // mask[i].resize(chunk_size / 8 / (input_vector_width / 8));
        job[i]->op                 = qpl_op_scan_range;
        job[i]->next_in_ptr        = decompressed_result_vector.data() + source_idx;
        job[i]->next_out_ptr       = mask[i].data();
        job[i]->available_in       = static_cast<uint32_t>(num_input);
        job[i]->available_out      = static_cast<uint32_t>(chunk_size);
        job[i]->src1_bit_width     = input_vector_width;
        job[i]->num_input_elements = static_cast<uint32_t>(num_input / (input_vector_width / 8));
        job[i]->out_bit_width      = qpl_ow_nom;
        job[i]->param_low          = lower_boundary;
        job[i]->param_high         = upper_boundary;

        source_idx += num_input;
        scan_left -= num_input;
        enqueue_cnt ++;
        if (scan_left == 0) {break;}
        
    }
    for (int i = 0; i < enqueue_cnt; ++i) {
        status = qpl_submit_job(job[i]);
        if (status != QPL_STS_OK) {
            std::cout << "An error " << status << " acquired during job execution." << std::endl;
            return 1;
        }
    }
    for (int i = 0; i < enqueue_cnt; ++i) {
        status = qpl_wait_job(job[i]);
        if (status != QPL_STS_OK) {
            std::cout << "An error " << status << " acquired during job waiting." << std::endl;
            return 1;
        }
    }
    return 1;
}

void iaa_select (std::vector<qpl_job *>& job, std::size_t& select_source_current_idx, int enqueue_cnt, size_t scan_left, std::vector<uint8_t> input_vector, std::vector<std::vector<uint8_t>> bitmap, std::vector<std::vector<uint8_t>>& dest_vector, uint32_t bit_width, std::vector<uint32_t> mask_length)
{
    uint32_t num_input = 0;
    qpl_status status;
    // auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < enqueue_cnt; ++i) {
        num_input = scan_left < chunk_size ? scan_left : chunk_size;
        // dest_vector[i].resize(chunk_size);
        job[i]->op                 = qpl_op_select;
        job[i]->next_in_ptr        = input_vector.data() + select_source_current_idx;
        job[i]->next_out_ptr       = dest_vector[i].data();
        job[i]->available_in       = static_cast<uint32_t>(num_input);
        job[i]->available_out      = static_cast<uint32_t>(chunk_size);
        job[i]->src1_bit_width     = bit_width;
        job[i]->num_input_elements = static_cast<uint32_t>(num_input) / (bit_width / 8);
        job[i]->next_src2_ptr      = bitmap[i].data();
        job[i]->available_src2     = mask_length[i];
        job[i]->src2_bit_width     = 1;
        select_source_current_idx += num_input;
        scan_left -= num_input;
        if (scan_left == 0) {break;}
    }
    for (int i = 0; i < enqueue_cnt; ++i) {
        status = qpl_submit_job(job[i]);
        if (status != QPL_STS_OK) {
            std::cout << "An error " << status << " acquired during job execution." << std::endl;
            return;
        }
    }
    for (int i = 0; i < enqueue_cnt; ++i) {
        status = qpl_wait_job(job[i]);
        if (status != QPL_STS_OK) {
            std::cout << "An error " << status << " acquired during job waiting." << std::endl;
            return;
        }
    }
    // auto end = std::chrono::high_resolution_clock::now();
    // std::chrono::duration<int64_t, std::nano> elapsed_time_ns = std::chrono::nanoseconds::zero();
    // elapsed_time_ns = end - start;
    // return static_cast<double>(elapsed_time_ns.count()) / 1000 / 1000 / 1000;
}

double iaa_decompression(std::string src_data_file_path, std::string dest_data_file_path, uint32_t iteration, const uint32_t queue_size, std::vector<qpl_job *>& job, std::vector<uint8_t>& decompressed_result_vector)
{
    qpl_path_t execution_path = qpl_path_hardware;
    std::vector<std::vector<uint8_t>> src_vector;

    // Opening destination file
    // std::ofstream dest_file;
    // dest_file.open(dest_data_file_path, std::ofstream::out | std::ofstream::binary);
    // if (!dest_file) {
    //     std::cout << "File not found : " << dest_data_file_path << std::endl;
    //     return 1;
    // }

    qpl_status                              status;

    // Allocation
    src_vector.resize(queue_size);
    decompressed_result_vector.resize(iteration*chunk_size);

    std::chrono::duration<int64_t, std::nano> elapsed_time_ns = std::chrono::nanoseconds::zero();
    std::size_t total_src_file_size = 0;
    std::size_t decompressed_size = 0;
    // Decompression
    for(uint32_t file_id = 0; file_id < iteration;) {
        int enqueue_cnt = 0;
        for (int i = 0; i < queue_size; ++i) {
            // Opening source file
            std::ifstream src_file;
            src_file.open(src_data_file_path + "." + std::to_string(file_id), std::ifstream::in | std::ifstream::binary);
            if (!src_file) {
                std::cout << "File not found : " << src_data_file_path + "." + std::to_string(file_id) << std::endl;
                return 1;
            }
            // Getting source file size
            src_file.seekg(0, std::ios::end);
            std::size_t src_file_size = static_cast<std::size_t>(src_file.tellg());
            src_file.seekg(0, std::ios::beg);

            // Resizing source and destination vector
            src_vector[i].resize(src_file_size);

            // Loading data from source file to source vector
            src_file.read(reinterpret_cast<char *>(&src_vector[i].front()), src_file_size);

            // Closing source file
            src_file.close();

            // Performing a operation
            job[i]->op             = qpl_op_decompress;
            job[i]->level          = qpl_default_level;
            job[i]->next_in_ptr    = src_vector[i].data();
            job[i]->next_out_ptr   = decompressed_result_vector.data() + chunk_size * file_id;
            job[i]->available_in   = src_file_size;
            job[i]->available_out  = chunk_size;
            job[i]->flags          = QPL_FLAG_FIRST | QPL_FLAG_OMIT_VERIFY | QPL_FLAG_LAST;

            total_src_file_size += src_file_size;
            enqueue_cnt++;
            ++file_id;
            if (file_id >= iteration) { break; }
        }

        // Submit & Waiting
        if(execution_path == qpl_path_software) {
            auto start = std::chrono::high_resolution_clock::now();
            for (int i = 0; i < enqueue_cnt; ++i) {
                status = qpl_execute_job(job[i]);
                if (status != QPL_STS_OK) {
                    std::cout << "An error " << status << " acquired during job execution." << std::endl;
                    return 1;
                }
            }
            // std::vector<std::thread *> job_th;
            // job_th.resize(enqueue_cnt);
            // for (int i = 0; i < enqueue_cnt; ++i) { job_th[i] = new std::thread(job_execution, job[i]); }
            // for (int i = 0; i < enqueue_cnt; ++i) { job_th[i]->join(); }
            // for (int i = 0; i < enqueue_cnt; ++i) { delete job_th[i]; }
            auto end = std::chrono::high_resolution_clock::now();
            elapsed_time_ns += end - start;
        } else {
            auto start = std::chrono::high_resolution_clock::now();
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
                if (status != QPL_STS_OK) {
                    std::cout << "An error " << status << " acquired during job waiting." << std::endl;
                    return 1;
                }
            }
            auto end = std::chrono::high_resolution_clock::now();
            elapsed_time_ns += end - start;
        }

        for (int i = 0; i < enqueue_cnt; ++i) {
            decompressed_size += static_cast<std::size_t>(job[i]->total_out);
        }

        // std::cout << '\r';
        // std::cout << "Progress ... " << (file_id) << " / " << iteration << " Files" << std::flush;
    }
    decompressed_result_vector.resize(decompressed_size);
    // dest_file.write(reinterpret_cast<char *>(&decompressed_result_vector.front()), decompressed_size);
    // Closing destination file
    // dest_file.close();
    double elapsed_time_sec = static_cast<double>(elapsed_time_ns.count()) / 1000 / 1000 / 1000;
    return elapsed_time_sec;
}

int query_processing(std::string src_data_file_dir, std::string dest_data_file_dir, qpl_path_t execution_path, const uint32_t queue_size)
{
    std::cout << "[IAA without Pipelining Functionality]" << std::endl;
    // std::vector<uint32_t> iteration(16, 0);
    // std::vector<std::string> columns = {
    //     "l_orderkey", "l_partkey", "l_suppkey", "l_linenumber", 
    //     "l_quantity", "l_extendedprice", "l_discount", "l_tax", 
    //     "l_returnflag", "l_linestatus", "l_shipdate", "l_commitdate", 
    //     "l_receiptdate", "l_shipinstruct", "l_shipmode", "l_comment"
    // };
    std::vector<uint32_t> iteration(4, 0);
    std::vector<std::string> columns = {
        "l_discount", "l_quantity", "l_shipdate", "l_extendedprice"
    };
    

    for (const auto& entry : std::filesystem::directory_iterator(src_data_file_dir)) {
        if (entry.is_regular_file()) {
            const std::string filename = entry.path().filename().string();
            
            // Check each column for a match
            for (size_t i = 0; i < columns.size(); ++i) {
                // Create the regex pattern for each column
                std::regex pattern(R"(.*)" + columns[i] + R"(\.bin\.iaa\.compressed\..*)");
                
                // Check if the filename matches the regex pattern
                if (std::regex_search(filename, pattern)) {
                    // Increment the corresponding element in the vector
                    iteration[i]++;
                }
            }
        }
    }

    // Job initialization
    std::vector<std::unique_ptr<uint8_t[]>> job_buffer;
    std::vector<qpl_job *>                  job;
    qpl_status                              status;
    uint32_t                                size = 0;

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

    std::vector<std::vector<uint8_t> >decompressed_result_vector;
    // decompressed_result_vector.resize(16);
    decompressed_result_vector.resize(4);
    double decompress_elapsed_time_sec = 0;
    for (size_t i = 0; i < columns.size(); i++) {
        std::string src_data_file_path = src_data_file_dir + columns[i] + ".bin.iaa.compressed";
        std::string dest_data_file_path = dest_data_file_dir + columns[i] + ".bin.iaa.decompressed";
        double r = iaa_decompression(src_data_file_path, dest_data_file_path, iteration[i], queue_size, job, decompressed_result_vector[i]);
        if (r == -1) break;
        decompress_elapsed_time_sec += r;
    }
    std::cout << "[Result Summary]" << std::endl;
    std::cout <<  "(Decompress) Time take: " << decompress_elapsed_time_sec << " sec" << std::endl;

// **************************************************************************************************************************************SCAN && SELECT **********************************************************************************************************************************************************************************************************************************************
    
    // Start Scanning && Select
    // std::cout << "Scan Start" << std::endl;
    // std::size_t scan_left = decompressed_result_vector[10].size();
    std::size_t scan_left = decompressed_result_vector[2].size();

    std::size_t shipdate_source_idx = 0;
    std::size_t discount_source_idx = 0;
    std::size_t quantity_source_idx = 0;
    std::size_t select_source_current_idx1 = 0;
    std::size_t select_source_current_idx2 = 0;
    uint32_t num_input = 0;
    std::vector<std::vector<uint8_t>> dest_vector1;
    std::vector<std::vector<uint8_t>> dest_vector2;
    std::vector<std::vector<uint8_t>> shipdate_mask;
    std::vector<std::vector<uint8_t>> discount_mask;
    std::vector<std::vector<uint8_t>> quantity_mask;
    std::vector<std::vector<uint8_t>> combination_mask1;
    std::vector<std::vector<uint8_t>> combination_mask2;

    // std::vector<uint8_t> temp;
    // temp.resize(decompressed_result_vector[6].size());
    // for (auto i = 0; i < decompressed_result_vector[6].size(); i++) {
    //     temp[i] = decompressed_result_vector[6][i];
    // }
    std::vector<uint8_t> temp;
    temp.resize(decompressed_result_vector[0].size());
    for (auto i = 0; i < decompressed_result_vector[0].size(); i++) {
        temp[i] = decompressed_result_vector[0][i];
    }

    dest_vector1.resize(queue_size);
    dest_vector2.resize(queue_size);
    shipdate_mask.resize(queue_size);
    discount_mask.resize(queue_size);
    quantity_mask.resize(queue_size);
    combination_mask1.resize(queue_size);
    combination_mask2.resize(queue_size);
    for (uint32_t i = 0; i < queue_size; i++) {
        shipdate_mask[i].resize(chunk_size);
        discount_mask[i].resize(chunk_size);
        quantity_mask[i].resize(chunk_size);
        dest_vector1[i].resize(chunk_size);
        dest_vector2[i].resize(chunk_size);
    }
    std::vector<uint8_t> result;
    float l_extendedprice = 0;
    float l_discount = 0;
    double sum = 0;
    uint32_t total_byte = 0;
    std::chrono::duration<int64_t, std::nano> scan_elapsed_time_ns = std::chrono::nanoseconds::zero();
    std::chrono::duration<int64_t, std::nano> mask_elapsed_time_ns = std::chrono::nanoseconds::zero();
    std::chrono::duration<int64_t, std::nano> select_elapsed_time_ns = std::chrono::nanoseconds::zero();
    std::chrono::duration<int64_t, std::nano> sum_elapsed_time_ns = std::chrono::nanoseconds::zero();
    // auto start = std::chrono::high_resolution_clock::now();
    while (scan_left > 0) {
        auto start = std::chrono::high_resolution_clock::now();
        // int enqueue_cnt = 0;
        // iaa_scan(job, scan_left, enqueue_cnt, queue_size, shipdate_mask, shipdate_source_idx, decompressed_result_vector[10], shipdate_lower_boundary, shipdate_upper_boundary);
        // enqueue_cnt = 0;
        // iaa_scan(job, scan_left, enqueue_cnt, queue_size, discount_mask, discount_source_idx, decompressed_result_vector[6], discount_lower_boundary, discount_upper_boundary);
        // enqueue_cnt = 0;
        // iaa_scan(job, scan_left, enqueue_cnt, queue_size, quantity_mask, quantity_source_idx, decompressed_result_vector[4], quantity_lower_boundary, quantity_upper_boundary);
        int enqueue_cnt = 0;
        iaa_scan(job, scan_left, enqueue_cnt, queue_size, shipdate_mask, shipdate_source_idx, decompressed_result_vector[2], shipdate_lower_boundary, shipdate_upper_boundary);
        enqueue_cnt = 0;
        iaa_scan(job, scan_left, enqueue_cnt, queue_size, discount_mask, discount_source_idx, decompressed_result_vector[0], discount_lower_boundary, discount_upper_boundary);
        enqueue_cnt = 0;
        iaa_scan(job, scan_left, enqueue_cnt, queue_size, quantity_mask, quantity_source_idx, decompressed_result_vector[1], quantity_lower_boundary, quantity_upper_boundary);
        auto end = std::chrono::high_resolution_clock::now();
        scan_elapsed_time_ns += end - start;

        std::vector<uint32_t> mask_length;
        mask_length.resize(enqueue_cnt);
        start = std::chrono::high_resolution_clock::now();
        for (int j = 0; j < enqueue_cnt; j++) {
            mask_length[j] = job[j]->total_out;
            std::size_t s = job[j]->total_out;
            combination_mask1[j].resize(s);
            // combination_mask2[j].resize(s);
            for (auto k = 0; k < s; k++) {
                combination_mask1[j][k] = shipdate_mask[j][k] & discount_mask[j][k] & quantity_mask[j][k];
                // combination_mask2[j][k] = shipdate_mask[j][k] & discount_mask[j][k] & quantity_mask[j][k];
            }
        }
        end = std::chrono::high_resolution_clock::now();
        mask_elapsed_time_ns += end - start;

        start = std::chrono::high_resolution_clock::now();
        // iaa_select (job, select_source_current_idx1, enqueue_cnt, scan_left, decompressed_result_vector[5], combination_mask1, dest_vector1, 32, mask_length);
        // iaa_select (job, select_source_current_idx2, enqueue_cnt, scan_left, decompressed_result_vector[6], combination_mask1, dest_vector2, 32, mask_length);
        iaa_select (job, select_source_current_idx1, enqueue_cnt, scan_left, decompressed_result_vector[3], combination_mask1, dest_vector1, 32, mask_length);
        iaa_select (job, select_source_current_idx2, enqueue_cnt, scan_left, decompressed_result_vector[0], combination_mask1, dest_vector2, 32, mask_length);
        end = std::chrono::high_resolution_clock::now();
        select_elapsed_time_ns += end - start;

        start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < enqueue_cnt; i++) {
            // std::cout <<"total_out: " << job[i]->total_out << std::endl;
            for (uint32_t j = 0; j < job[i]->total_out / 4; j++) {
                uint32_t value1 = (dest_vector1[i][j * 4 + 0] << 0)  |  // Least significant byte
                        (dest_vector1[i][j * 4 + 1] << 8)  |
                        (dest_vector1[i][j * 4 + 2] << 16) |
                        (dest_vector1[i][j * 4 + 3] << 24);  // Most significant byte
                uint32_t value2 = (dest_vector2[i][j * 4 + 0] << 0)  |  // Least significant byte
                        (dest_vector2[i][j * 4 + 1] << 8)  |
                        (dest_vector2[i][j * 4 + 2] << 16) |
                        (dest_vector2[i][j * 4 + 3] << 24);  // Most significant byte
                float f1 = *reinterpret_cast<float*>(&value1);
                float f2 = *reinterpret_cast<float*>(&value2);
                sum += f1 * f2;
            }
            total_byte += job[i]->total_out;
        }
        for (int i = 0; i < queue_size; ++i) {
            std::size_t num_input = scan_left < chunk_size ? scan_left : chunk_size;
            scan_left -= num_input;
            if (scan_left == 0) break;
        }
        end = std::chrono::high_resolution_clock::now();
        sum_elapsed_time_ns += end - start;
    }
    double scan_elapsed_time_sec = static_cast<double>(scan_elapsed_time_ns.count()) / 1000 / 1000 / 1000;
    double mask_elapsed_time_sec = static_cast<double>(mask_elapsed_time_ns.count()) / 1000 / 1000 / 1000;
    double select_elapsed_time_sec = static_cast<double>(select_elapsed_time_ns.count()) / 1000 / 1000 / 1000;
    double sum_elapsed_time_sec = static_cast<double>(sum_elapsed_time_ns.count()) / 1000 / 1000 / 1000;
    double total_elapsed_time_sec = scan_elapsed_time_sec + mask_elapsed_time_sec + select_elapsed_time_sec + sum_elapsed_time_sec;
    // std::cout << "l_extendedprice: " << l_extendedprice << std::endl;
    // std::cout << "l_discount: " << l_discount << std::endl;
    std::cout <<  "(Scan) Time take: " << scan_elapsed_time_sec << " sec" << std::endl;
    std::cout <<  "(Mask) Time take: " << mask_elapsed_time_sec << " sec" << std::endl;
    std::cout <<  "(Select) Time take: " << select_elapsed_time_sec << " sec" << std::endl;
    std::cout <<  "(Sum) Time take: " << sum_elapsed_time_sec << " sec" << std::endl;
    std::cout <<  "(Total) Time take: " << total_elapsed_time_sec << " sec" << std::endl;
    // std::cout << "sum: " << sum << std::endl;

    // Freeing resources
    for (int i = 0; i < queue_size; ++i) {
        status = qpl_fini_job(job[i]);
        if (status != QPL_STS_OK) {
            std::cout << "An error " << status << " acquired during job finalization." << std::endl;
            return 1;
        }
    }
    
    return 0;
}
int pipeliined_query_processing(std::string src_data_file_dir, std::string dest_data_file_dir, qpl_path_t execution_path, const uint32_t queue_size)
{
    std::cout << "[IAA with Pipelining Functionality]" << std::endl;
    std::vector<uint32_t> iteration(4, 0);
    std::vector<std::string> columns = {
        "l_discount", "l_quantity", "l_shipdate", "l_extendedprice"
    };
    

    for (const auto& entry : std::filesystem::directory_iterator(src_data_file_dir)) {
        if (entry.is_regular_file()) {
            const std::string filename = entry.path().filename().string();
            
            // Check each column for a match
            for (size_t i = 0; i < columns.size(); ++i) {
                // Create the regex pattern for each column
                std::regex pattern(R"(.*)" + columns[i] + R"(\.bin\.iaa\.compressed\..*)");
                
                // Check if the filename matches the regex pattern
                if (std::regex_search(filename, pattern)) {
                    // Increment the corresponding element in the vector
                    iteration[i]++;
                }
            }
        }
    }

    // Job initialization
    std::vector<std::unique_ptr<uint8_t[]>> job_buffer;
    std::vector<qpl_job *>                  job;
    std::vector<std::unique_ptr<uint8_t[]>> sel_job_buffer;
    std::vector<qpl_job *>                  sel_job;
    qpl_status                              status;
    uint32_t                                size = 0;

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
    sel_job_buffer.resize(queue_size);
    sel_job.resize(queue_size);
    status = qpl_get_job_size(execution_path, &size);
    if (status != QPL_STS_OK) {
        std::cout << "An error " << status << " acquired during job size getting." << std::endl;
        return 1;
    }
    for (int i = 0; i < queue_size; ++i) {
        sel_job_buffer[i] = std::make_unique<uint8_t[]>(size);
        sel_job[i] = reinterpret_cast<qpl_job *>(sel_job_buffer[i].get());
        status = qpl_init_job(execution_path, sel_job[i]);
        if (status != QPL_STS_OK) {
            std::cout << "An error " << status << " acquired during job initializing." << std::endl;
            return 1;
        }
    }

    double scan_elapsed_time_sec = 0;
    std::chrono::duration<int64_t, std::nano> mask_elapsed_time_ns = std::chrono::nanoseconds::zero();
    double select_elapsed_time_sec = 0;
    std::chrono::duration<int64_t, std::nano> sum_elapsed_time_ns = std::chrono::nanoseconds::zero();
    std::chrono::duration<int64_t, std::nano> file_read_elapsed_time_ns = std::chrono::nanoseconds::zero();

    std::vector<std::vector<uint8_t>> shipdate_mask;
    std::vector<std::vector<uint8_t>> discount_mask;
    std::vector<std::vector<uint8_t>> quantity_mask;
    std::vector<std::vector<uint8_t>> combination_mask1;
    std::vector<std::vector<uint8_t>> extend_result;
    std::vector<std::vector<uint8_t>> discount_result;
    std::vector<uint32_t> total_out;
    std::vector<uint32_t> mask_length;
    shipdate_mask.resize(iteration[2]);
    mask_length.resize(iteration[2]);
    extend_result.resize(iteration[2]);
    discount_result.resize(iteration[2]);
    total_out.resize(iteration[2]);
    discount_mask.resize(iteration[2]);
    quantity_mask.resize(iteration[2]);
    for (uint32_t i = 0; i < iteration[2]; i++) {
        shipdate_mask[i].resize(chunk_size);
        discount_mask[i].resize(chunk_size);
        quantity_mask[i].resize(chunk_size);
        extend_result[i].resize(chunk_size);
        discount_result[i].resize(chunk_size);
    }
    std::string orig_file_path = "/home/jykang5/tpc_h_100g/lineitem_data/";
    // shipdate
    std::string shipdate_src_file_path = src_data_file_dir + columns[2] + ".bin.iaa.compressed";
    uint32_t shipdate_iteration = iteration[2];
    std::string shipdate_orig_file_path = orig_file_path + columns[2] + ".bin";
    int fileSize = 0;
    getFileSize(shipdate_orig_file_path, &fileSize);
    // baseline_iaa_decompress_scan(shipdate_src_file_path, shipdate_iteration, queue_size, job, shipdate_mask, shipdate_lower_boundary, shipdate_upper_boundary, &fileSize, mask_length);
    double res = iaa_decompress_scan(shipdate_src_file_path, shipdate_iteration, queue_size, job, shipdate_mask, shipdate_lower_boundary, shipdate_upper_boundary, &fileSize, mask_length);
    if(res == -1) {return 0;}
    scan_elapsed_time_sec += res;
    // discount
    std::string discount_src_file_path = src_data_file_dir + columns[0] + ".bin.iaa.compressed";
    uint32_t discount_iteration = iteration[0];
    std::string discount_orig_file_path = orig_file_path + columns[0] + ".bin";
    fileSize = 0;
    getFileSize(discount_orig_file_path, &fileSize);
    res = iaa_decompress_scan(discount_src_file_path, discount_iteration, queue_size, job, discount_mask, discount_lower_boundary, discount_upper_boundary, &fileSize, mask_length);
    if(res == -1) {return 0;}
    scan_elapsed_time_sec += res;
    // quantity
    std::string quantity_src_file_path = src_data_file_dir + columns[1] + ".bin.iaa.compressed";
    uint32_t quantity_iteration = iteration[1];
    std::string quantity_orig_file_path = orig_file_path + columns[1] + ".bin";
    fileSize = 0;
    getFileSize(quantity_orig_file_path, &fileSize);
    res = iaa_decompress_scan(quantity_src_file_path, quantity_iteration, queue_size, job, quantity_mask, quantity_lower_boundary, quantity_upper_boundary, &fileSize, mask_length);
    if(res == -1) {return 0;}
    scan_elapsed_time_sec += res;
    
    combination_mask1.resize(shipdate_iteration);
    for (int j = 0; j < shipdate_iteration; j++) {
        std::size_t s = mask_length[j]; //shipdate_mask[j].size();
        combination_mask1[j].resize(chunk_size);
        auto start = std::chrono::high_resolution_clock::now();
        for (auto k = 0; k < s; k++) {
            combination_mask1[j][k] = shipdate_mask[j][k] & discount_mask[j][k] & quantity_mask[j][k];
        }
        auto end = std::chrono::high_resolution_clock::now();
        mask_elapsed_time_ns += end - start;
    }

    // discount
    std::string sel_discount_src_file_path = src_data_file_dir + columns[0] + ".bin.iaa.compressed";
    uint32_t sel_discount_iteration = iteration[0];
    std::string sel_discount_orig_file_path = orig_file_path + columns[0] + ".bin";
    fileSize = 0;
    getFileSize(sel_discount_orig_file_path, &fileSize);
    res = iaa_decompress_select(sel_discount_src_file_path, sel_discount_iteration, queue_size, sel_job, discount_result, &fileSize, combination_mask1, 32, mask_length, total_out);
    if (res == -1) {return 0;}
    select_elapsed_time_sec += res;
    // extended_price
    std::string sel_extended_price_src_file_path = src_data_file_dir + columns[3] + ".bin.iaa.compressed";
    uint32_t sel_extended_price_iteration = iteration[3];
    std::string sel_extended_price_orig_file_path = orig_file_path + columns[3] + ".bin";
    fileSize = 0;
    getFileSize(sel_extended_price_orig_file_path, &fileSize);
    res = iaa_decompress_select(sel_extended_price_src_file_path, sel_extended_price_iteration, queue_size, sel_job, extend_result, &fileSize, combination_mask1, 32, mask_length, total_out);
    float aggregation_sum = 0.0;
    for (int i = 0; i < queue_size; ++i) {
        uint32_t sum_value = sel_job[i]->sum_value;
        aggregation_sum += static_cast<float>(sum_value);
    }
    float t_s = 0.0;
    float just_sum = 0.0;
    if (res == -1) {return 0;}
    select_elapsed_time_sec += res;
    float sum = 0;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iteration[2]; i++) {
        for (uint32_t j = 0; j < total_out[i] / 4; j++) {
            uint32_t value1 = (discount_result[i][j * 4 + 0] << 0)  |  // Least significant byte
                    (discount_result[i][j * 4 + 1] << 8)  |
                    (discount_result[i][j * 4 + 2] << 16) |
                    (discount_result[i][j * 4 + 3] << 24);  // Most significant byte
            uint32_t value2 = (extend_result[i][j * 4 + 0] << 0)  |  // Least significant byte
                    (extend_result[i][j * 4 + 1] << 8)  |
                    (extend_result[i][j * 4 + 2] << 16) |
                    (extend_result[i][j * 4 + 3] << 24);  // Most significant byte
            float f1 = *reinterpret_cast<float*>(&value1);
            float f2 = *reinterpret_cast<float*>(&value2);
            uint32_t value = 0;
            value = extend_result[i][j * 4 + 0] + extend_result[i][j * 4 + 1] + extend_result[i][j * 4 + 2] + extend_result[i][j * 4 + 3];
            t_s += static_cast<float>(value);
            just_sum += f2;
            sum += f1*f2;
        }
    }
    auto end = std::chrono::high_resolution_clock::now();

    sum_elapsed_time_ns += end - start;
    auto file_start = std::chrono::high_resolution_clock::now();
    std::ifstream file(shipdate_orig_file_path, std::ios::binary | std::ios::ate);
    if (file){
        std::streamsize size = file.tellg();
        file.close();
    }
    std::ifstream file1(discount_orig_file_path, std::ios::binary | std::ios::ate);
    if (file1){
        std::streamsize size = file1.tellg();
        file1.close();
    }
    std::ifstream file2(quantity_src_file_path, std::ios::binary | std::ios::ate);
    if (file2){
        std::streamsize size = file2.tellg();
        file2.close();
    }
    std::ifstream file3(sel_extended_price_orig_file_path, std::ios::binary | std::ios::ate);
    if (file3){
        std::streamsize size = file3.tellg();
        file3.close();
    }
    auto file_end = std::chrono::high_resolution_clock::now();
    file_read_elapsed_time_ns = file_end - file_start;

    double mask_elapsed_time_sec = static_cast<double>(mask_elapsed_time_ns.count()) / 1000 / 1000 / 1000;
    double sum_elapsed_time_sec = static_cast<double>(sum_elapsed_time_ns.count()) / 1000 / 1000 / 1000;
    double total_elapsed_time_sec = scan_elapsed_time_sec + mask_elapsed_time_sec + select_elapsed_time_sec + sum_elapsed_time_sec;
    std::cout << "[Result Summary]" << std::endl;
    std::cout <<  "(Scan) Time take: " << scan_elapsed_time_sec << " sec" << std::endl;
    std::cout <<  "(Mask) Time take: " << mask_elapsed_time_sec << " sec" << std::endl;
    std::cout <<  "(Select) Time take: " << select_elapsed_time_sec << " sec" << std::endl;
    std::cout <<  "(Sum) Time take: " << sum_elapsed_time_sec << " sec" << std::endl;
    std::cout <<  "(Total) Time take: " << total_elapsed_time_sec << " sec" << std::endl;
    std::cout << "sum: " << sum << std::endl;
    double file_read_elapsed_time_sec = static_cast<double>(file_read_elapsed_time_ns.count()) / 1000 / 1000 / 1000;
    std::cout <<  "(file_read) Time take: " << file_read_elapsed_time_sec << " sec" << std::endl;

    
    for (int i = 0; i < queue_size; ++i) {
        status = qpl_fini_job(job[i]);
        if (status != QPL_STS_OK) {
            std::cout << "An error " << status << " acquired during job finalization." << std::endl;
            return 1;
        }
        status = qpl_fini_job(sel_job[i]);
        if (status != QPL_STS_OK) {
            std::cout << "An error " << status << " acquired during job finalization." << std::endl;
            return 1;
        }
    }
    
    return 0;
}

auto main(int argc, char** argv) -> int {
    std::cout << std::endl;
    std::cout << "Intel(R) Query Processing Library version is " << qpl_get_library_version() << ".\n";

    // Default tos Software Path
    qpl_path_t execution_path = qpl_path_hardware;

    // Get path from input argument
    int parse_ret = parse_execution_path(argc, argv, &execution_path, 1);
    if (parse_ret != 0) {
        return 1;
    }

    const std::string COMPRESSED_DATA_FILE_DIR    = argv[2];
    const std::string DECOMPRESSED_DATA_FILE_NAME    = "";// argv[3];
    const uint32_t queue_size = static_cast<uint32_t>(atoi(argv[3]));

    // Decompression
    if(query_processing(COMPRESSED_DATA_FILE_DIR, DECOMPRESSED_DATA_FILE_NAME, execution_path, queue_size) != 0) {
        std::cout << "An error acquired during iaa_execution(decompression)" << std::endl;
        return 1;
    }
    std::cout << "==========================================================================" << std::endl;
    if (execution_path == qpl_path_software) {
        std::cout << std::endl;
        return 0;
    }
    if(pipeliined_query_processing(COMPRESSED_DATA_FILE_DIR, DECOMPRESSED_DATA_FILE_NAME, execution_path, queue_size) != 0) {
        std::cout << "An error acquired during iaa_execution(decompression)" << std::endl;
        return 1;
    }

    std::cout << std::endl;

    return 0;
}

//* [QPL_LOW_LEVEL_COMPRESSION_EXAMPLE] */
