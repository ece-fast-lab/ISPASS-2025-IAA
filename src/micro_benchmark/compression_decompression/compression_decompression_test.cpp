//* [QPL_LOW_LEVEL_COMPRESSION_EXAMPLE] */

#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <memory>
#include <string>
#include <chrono>
#include <thread>
#include <future>

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

int iaa_compression(std::string src_data_file_path, std::string dest_data_file_path, qpl_path_t execution_path, uint32_t &iteration, const uint32_t queue_size)
{
    // Source and output containers
    std::vector<uint8_t> whole_src_vector;
    std::vector<std::vector<uint8_t>> src_vector;
    std::vector<std::vector<uint8_t>> dest_vector;
    double elapsed_time_sec = 0;

    std::cout << "[IAA Compression]" << std::endl;
    
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
    src_vector.resize(queue_size);
    dest_vector.resize(queue_size);
    job_buffer.resize(queue_size);
    job.resize(queue_size);
    auto init_start = std::chrono::steady_clock::now();
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
    auto init_end = std::chrono::steady_clock::now();

    std::chrono::duration<int64_t, std::nano> init_time_ns = std::chrono::nanoseconds::zero();
    init_time_ns = init_end - init_start;
    double int_time_sec = static_cast<double>(init_time_ns.count()) / 1000 / 1000 / 1000;
    std::cout << "Init Time = " << init_time_ns.count() << " ns (" << int_time_sec << " s)" << std::endl;

    std::chrono::duration<int64_t, std::nano> elapsed_time_ns = std::chrono::nanoseconds::zero();
    std::chrono::duration<int64_t, std::nano> whole_time_ns = std::chrono::nanoseconds::zero();
    std::chrono::duration<int64_t, std::nano> file_read_time_ns = std::chrono::nanoseconds::zero();

    std::size_t src_file_left = src_file_size;
    std::size_t compressed_size = 0;
    std::size_t vector_size = 0;
    iteration = 0;

    whole_src_vector.resize(src_file_size);
    for(int i = 0; i < queue_size; i++) {
        dest_vector[i].resize(chunk_size);
    }
    // // Load memory 
    auto start = std::chrono::steady_clock::now();
    src_file.read(reinterpret_cast<char *>(&whole_src_vector.front()), src_file_size);
    auto end = std::chrono::steady_clock::now();
    whole_time_ns += end - start;
    file_read_time_ns += end - start;
    // // Closing source file
    src_file.close();
    std::size_t current_idx = 0;

    uint8_t** dest_array = new uint8_t*[queue_size];
    for (int i = 0; i < queue_size; ++i) {
        dest_array[i] = new uint8_t[chunk_size]();
    }

    // Compression
    auto whole_start = std::chrono::steady_clock::now();
    while(src_file_left > 0) {
        int enqueue_cnt = 0;
        for (int i = 0; i < queue_size; ++i) {
            // Resizing source and destination vectors
            if (src_file_left <= chunk_size) {
                vector_size = src_file_left;
            } else {
                vector_size = chunk_size;
            }
            src_vector[i].resize(vector_size);
            // dest_vector[i].resize(vector_size);

            // Allocate Huffman table object (c_huffman_table).
            qpl_huffman_table_t c_huffman_table = nullptr;

            status = qpl_deflate_huffman_table_create(compression_table_type, execution_path, DEFAULT_ALLOCATOR_C,
                                                    &c_huffman_table);
            if (status != QPL_STS_OK) {
                std::cout << "An error " << status << " acquired during Huffman table creation.\n";
                return 1;
            }

            // Initialize Huffman table using deflate tokens histogram.
            qpl_histogram histogram {};
            status = qpl_gather_deflate_statistics(whole_src_vector.data() + current_idx, vector_size, &histogram, qpl_default_level, execution_path);
            if (status != QPL_STS_OK) {
                std::cout << "An error " << status << " acquired during gathering statistics for Huffman table.\n";
                qpl_huffman_table_destroy(c_huffman_table);
                return 1;
            }

            status = qpl_huffman_table_init_with_histogram(c_huffman_table, &histogram);
            if (status != QPL_STS_OK) {
                std::cout << "An error " << status << " acquired during Huffman table initialization.\n";
                qpl_huffman_table_destroy(c_huffman_table);
                return 1;
            }

            // Loading data from source file to source vector
            // src_file.read(reinterpret_cast<char *>(&src_vector[i].front()), vector_size);
            // Performing a operation
            job[i]->op             = qpl_op_compress;
            job[i]->level          = qpl_default_level;
            job[i]->next_in_ptr    = whole_src_vector.data() + current_idx; // src_vector[i].data();
            job[i]->next_out_ptr   = dest_vector[i].data();
            job[i]->available_in   = static_cast<uint32_t>(vector_size);
            job[i]->available_out  = static_cast<uint32_t>(vector_size);
            job[i]->flags          = QPL_FLAG_FIRST | QPL_FLAG_LAST | QPL_FLAG_OMIT_VERIFY ;
            job[i]->huffman_table  = c_huffman_table;

            current_idx += vector_size;

            src_file_left -= vector_size;
            enqueue_cnt = i + 1;
            if (src_file_left == 0) { break; }
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
                if (status != QPL_STS_OK) {
                    std::cout << "An error " << status << " acquired during job waiting." << std::endl;
                    return 1;
                }
            }
            auto end = std::chrono::steady_clock::now();
            elapsed_time_ns += end - start;
        }
        
        auto start = std::chrono::steady_clock::now();
        for (int i = 0; i < enqueue_cnt; ++i) {
            // std::cout << std::endl <<  job[i]->total_out << std::endl;
            compressed_size += static_cast<std::size_t>(job[i]->total_out);

            // Opening destination file
            std::ofstream dest_file;
            dest_file.open(dest_data_file_path + std::to_string(iteration + i), std::ofstream::out | std::ofstream::binary);
            if (!dest_file) {
                std::cout << "File not found : " << dest_data_file_path << std::endl;
                return 1;
            }

            // Writing compressed data to destination file
            dest_file.write(reinterpret_cast<char *>(&dest_vector[i].front()), static_cast<std::size_t>(job[i]->total_out));

            // Closing destination file
            dest_file.close();
        }
        auto end = std::chrono::steady_clock::now();
        whole_time_ns -= end - start;

        iteration += enqueue_cnt;

        // std::cout << '\r';
        // std::cout << "Progress ... " << (src_file_size - src_file_left) << " / " << src_file_size << " Bytes" << std::flush;
    }
    auto whole_end = std::chrono::steady_clock::now();
    whole_time_ns += whole_end - whole_start;

    // Closing source file
    src_file.close();
    

    // Freeing resources
    for (int i = 0; i < queue_size; ++i) {
        status = qpl_fini_job(job[i]);
        if (status != QPL_STS_OK) {
            std::cout << "An error " << status << " acquired during job finalization." << std::endl;
            return 1;
        }
    }

    std::cout << std::endl;
    std::cout << "Content was successfully compressed." << std::endl;
    std::cout << "Input size       = " << src_file_size << " Bytes" << std::endl;
    std::cout << "Output size      = " << compressed_size << " Bytes" << std::endl;
    std::cout << "Ratio            = " << static_cast<double>(compressed_size) / static_cast<double>(src_file_size) << std::endl;
    double whole_time_sec = static_cast<double>(whole_time_ns.count()) / 1000 / 1000 / 1000;
    // std::cout << "Whole Time = " << whole_time_ns.count() << " ns (" << whole_time_sec << " s)" << std::endl;
    double file_read_time_sec = static_cast<double>(file_read_time_ns.count()) / 1000 / 1000 / 1000;
    // std::cout << "File read Time = " << file_read_time_ns.count() << " ns (" << file_read_time_sec << " s)" << std::endl;
    elapsed_time_sec = static_cast<double>(elapsed_time_ns.count()) / 1000 / 1000 / 1000;
    // std::cout << "Elapsed Time = " << elapsed_time_ns.count() << " ns (" << elapsed_time_sec << " s)" << std::endl;
    std::cout << "Bandwidth        = " << static_cast<double>(src_file_size) / 1024 / 1024 / elapsed_time_sec << " MB/s" << std::endl;

    return 0;
}

int iaa_decompression(std::string src_data_file_path, std::string dest_data_file_path, qpl_path_t execution_path, uint32_t iteration, const uint32_t queue_size)
{
    // Source and output containers
    auto start = std::chrono::steady_clock::now();
    // Simulate workload
    auto end = std::chrono::steady_clock::now();
    auto elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    std::cout << "Elapsed time: " << elapsed_ns << " ns" << std::endl;
    double elapsed_time_sec = 0;

    std::vector<uint8_t> whole_src_vector;
    std::vector<std::vector<uint8_t>> src_vector;
    std::vector<std::vector<uint8_t>> dest_vector;

    std::cout << "[IAA Decompression]" << std::endl;

    // Opening destination file
    std::cout << "Making destination file... ";
    std::ofstream dest_file;
    dest_file.open(dest_data_file_path, std::ofstream::out | std::ofstream::binary);
    if (!dest_file) {
        std::cout << "File not found : " << dest_data_file_path << std::endl;
        return 1;
    }
    std::cout << "Done" << std::endl;

    // Job initialization
    std::vector<std::unique_ptr<uint8_t[]>> job_buffer;
    std::vector<qpl_job *>                  job;
    qpl_status                              status;
    uint32_t                                size = 0;

    // Allocation
    src_vector.resize(queue_size);
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

    std::chrono::duration<int64_t, std::nano> file_read_time_ns = std::chrono::nanoseconds::zero();
    std::chrono::duration<int64_t, std::nano> elapsed_time_ns = std::chrono::nanoseconds::zero();
    std::chrono::duration<int64_t, std::nano> whole_time_ns = std::chrono::nanoseconds::zero();
    std::size_t total_src_file_size = 0;
    std::size_t decompressed_size = 0;
    chunk_size = 2097152;
    for (int i = 0; i < queue_size; i++) {
        src_vector[i].resize(chunk_size);
        dest_vector[i].resize(chunk_size); // It can't be over chunk_size(2MB)
    }
    uint8_t** src_array = new uint8_t*[queue_size];
    uint8_t** dest_array = new uint8_t*[queue_size];
    for (int i = 0; i < queue_size; ++i) {
        dest_array[i] = new uint8_t[chunk_size]();
    }

    auto whole_start = std::chrono::steady_clock::now();
    for(uint32_t file_id = 0; file_id < iteration;) {
        int enqueue_cnt = 0;
        for (int i = 0; i < queue_size; ++i) {
            // Opening source file
            std::ifstream src_file;
            src_file.open(src_data_file_path + std::to_string(file_id), std::ifstream::in | std::ifstream::binary);
            if (!src_file) {
                std::cout << "File not found : " << src_data_file_path + "." + std::to_string(file_id) << std::endl;
                return 1;
            }
            
            // Getting source file size
            src_file.seekg(0, std::ios::end);
            std::size_t src_file_size = static_cast<std::size_t>(src_file.tellg());
            src_file.seekg(0, std::ios::beg);

            src_array[i] = new uint8_t[src_file_size];

            // Loading data from source file to source vector
            auto start = std::chrono::steady_clock::now();
            src_file.read(reinterpret_cast<char *>(&src_vector[i].front()), src_file_size);
            // src_file.read(reinterpret_cast<char *>(src_array[i]), src_file_size);
            auto end = std::chrono::steady_clock::now();
            file_read_time_ns += end - start;

            // Closing source file
            src_file.close();

            // Performing a operation
            job[i]->op             = qpl_op_decompress;
            job[i]->next_in_ptr    = src_vector[i].data();
            job[i]->next_out_ptr   = dest_array[i];// dest_vector[i].data();
            job[i]->available_in   = src_file_size;
            job[i]->available_out  = chunk_size;
            job[i]->flags          = QPL_FLAG_FIRST | QPL_FLAG_LAST;

            total_src_file_size += src_file_size;
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
                    std::cout << "An error " << status << " acquired during job execution." << std::endl;
                    return 1;
                }
            }
            for (int i = 0; i < enqueue_cnt; ++i) {
                auto start_t = std::chrono::steady_clock::now();
                status = qpl_wait_job(job[i]);
                if (status != QPL_STS_OK) {
                    std::cout << "An error " << status << " acquired during job waiting." << std::endl;
                    return 1;
                }
                auto end_t = std::chrono::steady_clock::now();
                std::chrono::duration<int64_t, std::nano> et = end_t - start_t;
                double elapsed_time_sec = static_cast<double>(et.count()) / 1000 / 1000;
                auto elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end_t - start_t).count();
            }
            auto end = std::chrono::steady_clock::now();
            elapsed_time_ns += end - start;
        }

        // auto start = std::chrono::steady_clock::now();
        for (int i = 0; i < enqueue_cnt; ++i) {
            decompressed_size += static_cast<std::size_t>(job[i]->total_out);
            // Writing compressed data to destination file
            dest_file.write(reinterpret_cast<char *>(&dest_vector[i].front()), static_cast<std::size_t>(job[i]->total_out));
        }
        // auto end = std::chrono::steady_clock::now();
        // whole_time_ns -= end - start;

        // std::cout << '\r';
        // std::cout << "Progress ... " << (file_id) << " / " << iteration << " Files" << std::flush;
    }
    auto whole_end = std::chrono::steady_clock::now();
    whole_time_ns += whole_end - whole_start;

    // Closing destination file
    dest_file.close();

    // Freeing resources
    for (int i = 0; i < queue_size; ++i) {
        status = qpl_fini_job(job[i]);
        if (status != QPL_STS_OK) {
            std::cout << "An error " << status << " acquired during job finalization." << std::endl;
            return 1;
        }
    }

    std::cout << std::endl;
    std::cout << "Content was successfully decompressed." << std::endl;
    std::cout << "Input size       = " << total_src_file_size << " Bytes" << std::endl;
    std::cout << "Output size      = " << decompressed_size << " Bytes" << std::endl;
    // std::cout << "Ratio            = " << static_cast<double>(decompressed_size) / static_cast<double>(total_src_file_size) << std::endl;
    

    double whole_time_sec = static_cast<double>(whole_time_ns.count()) / 1000 / 1000 / 1000;
    // std::cout << "Whole Time = " << whole_time_ns.count() << " ns (" << whole_time_sec << " s)" << std::endl;
    double file_read_time_sec = static_cast<double>(file_read_time_ns.count()) / 1000 / 1000 / 1000;
    // std::cout << "File read Time = " << file_read_time_ns.count() << " ns (" << file_read_time_sec << " s)" << std::endl;
    elapsed_time_sec = static_cast<double>(elapsed_time_ns.count()) / 1000 / 1000 / 1000;
    // std::cout << "Elapsed Time = " << elapsed_time_ns.count() << " ns (" << elapsed_time_sec << " s)" << std::endl;
    std::cout << "Bandwidth        = " << static_cast<double>(decompressed_size) / 1024 / 1024 / elapsed_time_sec << " MB/s" << std::endl;
    
    
    return 0;
}

auto main(int argc, char** argv) -> int {
    std::cout << std::endl;
    std::cout << "Intel(R) Query Processing Library version is " << qpl_get_library_version() << ".\n";

    // Default to Software Path
    qpl_path_t execution_path = qpl_path_software;

    std::string path = argv[1];
    if (path == "hardware_path") {
        execution_path = qpl_path_hardware;
        std::cout << "The test will be run on the hardware path." << std::endl;
    } else if (path == "software_path") {
        execution_path = qpl_path_software;
        std::cout << "The test will be run on the software path." << std::endl;
    } else {
        std::cout << "Unrecognized value for parameter. Use hardware_path or software_path." << std::endl;
        return 1;
    }

    // File path
    const std::string SRC_DATA_FILE_PATH    = argv[2];
    std::filesystem::path srcPath(SRC_DATA_FILE_PATH);
    std::string parentDir = srcPath.parent_path().string();
    std::string fileName = srcPath.filename().string();

     // Define new directories
    std::filesystem::path compressedDir = parentDir + "/compressed_data";
    std::filesystem::path decompressedDir = parentDir + "/decompressed_data";

    // Create directories if they don't exist
    if (!std::filesystem::exists(compressedDir)) {
        std::filesystem::create_directory(compressedDir);
        std::cout << "Created directory: " << compressedDir << std::endl;
    }

    if (!std::filesystem::exists(decompressedDir)) {
        std::filesystem::create_directory(decompressedDir);
        std::cout << "Created directory: " << decompressedDir << std::endl;
    }

    std::filesystem::path destFilePath = compressedDir / (fileName + ".iaa.compressed");
    const std::string DEST_DATA_FILE_PATH = destFilePath.string();
    std::filesystem::path refFilePath = decompressedDir / (fileName + ".iaa.decompressed");
    const std::string REF_DATA_FILE_PATH    = refFilePath.string();
    
    const uint32_t queue_size = static_cast<uint32_t>(atoi(argv[3]));
    uint32_t iteration = 0;

    chunk_size = static_cast<std::size_t>(atoi(argv[4]));

    std::cout << std::endl;
    // Compression
    if(iaa_compression(SRC_DATA_FILE_PATH, DEST_DATA_FILE_PATH, execution_path, iteration, queue_size) != 0) {
        std::cout << "An error acquired during iaa_execution(compression)" << std::endl;
        return 1;
    }

    std::cout << std::endl;
    // Decompression
    if(iaa_decompression(DEST_DATA_FILE_PATH, REF_DATA_FILE_PATH, execution_path, iteration, queue_size) != 0) {
        std::cout << "An error acquired during iaa_execution(decompression)" << std::endl;
        return 1;
    }

    std::cout << std::endl;

    return 0;
}

//* [QPL_LOW_LEVEL_COMPRESSION_EXAMPLE] */
