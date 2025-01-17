#include <stdlib.h>
#include <getopt.h>
#include <sys/stat.h>
#include <unistd.h> 

#include <iostream>
#include <fstream>
#include <thread>
#include <atomic>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <sstream>

#include "Common.hpp"
#include "BUSData.h"

#include "bustools_sort.h"
#include "bustools_count.h"
#include "bustools_whitelist.h"
#include "bustools_project.h"
#include "bustools_inspect.h"
#include "bustools_linker.h"
#include "bustools_capture.h"

int my_mkdir(const char *path, mode_t mode) {
  #ifdef _WIN64
  return mkdir(path);
  #else
  return mkdir(path,mode);
  #endif
}

bool checkFileExists(const std::string &fn) {
  struct stat stFileInfo;
  auto intStat = stat(fn.c_str(), &stFileInfo);
  return intStat == 0;
}

bool checkDirectoryExists(const std::string &fn) {
  struct stat stFileInfo;
  auto intStat = stat(fn.c_str(), &stFileInfo);
  return intStat == 0 && S_ISDIR(stFileInfo.st_mode);
}






void parse_ProgramOptions_sort(int argc, char **argv, Bustools_opt& opt) {

  const char* opt_string = "t:o:m:T:p";

  static struct option long_options[] = {
    {"threads",         required_argument,  0, 't'},
    {"output",          required_argument,  0, 'o'},
    {"memory",          required_argument,  0, 'm'},
    {"temp",            required_argument,  0, 'T'},
    {"pipe",            no_argument, 0, 'p'},
    {0,                 0,                  0,  0 }
  };

  int option_index = 0, c;

  while ((c = getopt_long(argc, argv, opt_string, long_options, &option_index)) != -1) {
    std::string s;
    size_t sh = 0;
    int n = 0;
    switch (c) {

    case 't':
      opt.threads = atoi(optarg);
      break;    
    case 'o':
      opt.output = optarg;
      break;
    case 'm':
      s = optarg;
      sh = 0;
      n = s.size();
      if (n==0) {
        break;
      }
      switch(s[n-1]) {
        case 'm':
        case 'M':
          sh = 20;
          n--;
          break;
        case 'g':
        case 'G':
          sh = 30;
          n--;
          break;
        default:
          sh = 0;
          break;
      }
      opt.max_memory = atoi(s.substr(0,n).c_str());
      opt.max_memory <<= sh;
      break;
    case 'T':
      opt.temp_files = optarg;
      break;
    case 'p':
      opt.stream_out = true;
      break;
    default:
      break;
    }
  }

  // all other arguments are fast[a/q] files to be read
  while (optind < argc) opt.files.push_back(argv[optind++]);
  
  if (opt.files.size() == 1 && opt.files[0] == "-") {
    opt.stream_in = true;
  }
}

void parse_ProgramOptions_merge(int argc, char **argv, Bustools_opt& opt) {
   const char* opt_string = "o:";

  static struct option long_options[] = {
    {"output",          required_argument,  0, 'o'},
    {0,                 0,                  0,  0 }
  };

  int option_index = 0, c;

  while ((c = getopt_long(argc, argv, opt_string, long_options, &option_index)) != -1) {

    switch (c) {
    case 'o':
      opt.output = optarg;
      break;
    default:
      break;
    }
  }

  while (optind < argc) opt.files.push_back(argv[optind++]);
}

void parse_ProgramOptions_capture(int argc, char **argv, Bustools_opt& opt) {
   const char* opt_string = "o:xc:e:t:subfp";

  static struct option long_options[] = {
    {"output",          required_argument,  0, 'o'},
    {"complement",      no_argument,        0, 'x'},
    {"capture",         required_argument,  0, 'c'},
    {"ecmap",           required_argument,  0, 'e'},
    {"txnames",         required_argument,  0, 't'},
    {"transcripts",     no_argument,        0, 's'},
    {"umis",            no_argument,        0, 'u'},
    {"barcode",         no_argument,        0, 'b'},
    {"combo",           no_argument,        0, 'f'},
    {"pipe",            no_argument,        0, 'p'},
    {0,                 0,                  0,  0 }
  };

  int option_index = 0, c;

  while ((c = getopt_long(argc, argv, opt_string, long_options, &option_index)) != -1) {

    switch (c) {
    case 'o':
      opt.output = optarg;
      break;
    case 'x':
      opt.complement = true;
      break;
    case 'c':
      opt.capture = optarg;
      break;
    case 'e':
      opt.count_ecs = optarg;
      break;
    case 't':
      opt.count_txp = optarg;
      break;
    case 's':
      opt.type = CAPTURE_TX;
      break;
    case 'u':
      opt.type = CAPTURE_UMI;
      break;
    case 'b':
      opt.type = CAPTURE_BC;
      break;
    case 'f':
      opt.filter = true;
      break;
    case 'p':
      opt.stream_out = true;
      break;
    default:
      break;
    }
  }

  while (optind < argc) opt.files.push_back(argv[optind++]);

  if (opt.files.size() == 1 && opt.files[0] == "-") {
    opt.stream_in = true;
  }
}

void parse_ProgramOptions_count(int argc, char **argv, Bustools_opt& opt) {
  const char* opt_string = "o:g:e:t:m";
  int gene_flag = 0;
  static struct option long_options[] = {
    {"output",          required_argument,  0, 'o'},
    {"genemap",          required_argument,  0, 'g'},
    {"ecmap",          required_argument,  0, 'e'},
    {"txnames",          required_argument,  0, 't'},
    {"genecounts", no_argument, &gene_flag, 1},
    {"multimapping", no_argument, 0, 'm'},
    {0,                 0,                  0,  0 }
  };

  int option_index = 0, c;

  while ((c = getopt_long(argc, argv, opt_string, long_options, &option_index)) != -1) {

    switch (c) {
    case 'o':
      opt.output = optarg;
      break;
    case 'g':
      opt.count_genes = optarg;
      break;
    case 't':
      opt.count_txp = optarg;
      break;
    case 'e':
      opt.count_ecs = optarg;
      break;
    case 'm':
      opt.count_gene_multimapping = true;
      break;
    default:
      break;
    }
  }
  if (gene_flag) {
    opt.count_collapse = true;
  }

  while (optind < argc) opt.files.push_back(argv[optind++]);

  if (opt.files.size() == 1 && opt.files[0] == "-") {
    opt.stream_in = true;
  }
}

void parse_ProgramOptions_dump(int argc, char **argv, Bustools_opt& opt) {

  const char* opt_string = "o:p";

  static struct option long_options[] = {
    {"output",          required_argument,  0, 'o'},
    {"pipe",            no_argument, 0, 'p'},
    {0,                 0,                  0,  0 }
  };

  int option_index = 0, c;

  while ((c = getopt_long(argc, argv, opt_string, long_options, &option_index)) != -1) {

    switch (c) {
    case 'o':
      opt.output = optarg;
      break;
    case 'p':
      opt.stream_out = true;
    default:
      break;
    }
  }

  // all other arguments are fast[a/q] files to be read
  while (optind < argc) opt.files.push_back(argv[optind++]);

  if (opt.files.size() == 1 && opt.files[0] == "-") {
    opt.stream_in = true;
  }
}

void parse_ProgramOptions_correct(int argc, char **argv, Bustools_opt& opt) {

  const char* opt_string = "o:w:p";
  static struct option long_options[] = {
    {"output",          required_argument,  0, 'o'},
    {"whitelist",       required_argument,  0, 'w'},
    {"pipe",            no_argument, 0, 'p'},
    {0,                 0,                  0,  0 }
  };

  int option_index = 0, c;

  while ((c = getopt_long(argc, argv, opt_string, long_options, &option_index)) != -1) {

    switch (c) {
    case 'o':
      opt.output = optarg;
      break;
    case 'w':
      opt.whitelist = optarg;
      break;
    case 'p':
      opt.stream_out = true;
      break;
    default:
      break;
    }
  }


  //hard code options for now
  opt.ec_d = 1;
  opt.ec_dmin = 3;

  // all other arguments are fast[a/q] files to be read
  while (optind < argc) opt.files.push_back(argv[optind++]);

  if (opt.files.size() == 1 && opt.files[0] == "-") {
    opt.stream_in = true;
  }
}

void parse_ProgramOptions_whitelist(int argc, char **argv, Bustools_opt &opt) {
  
  /* Parse options. */
  const char *opt_string = "o:f:";

  static struct option long_options[] = {
    {"output", required_argument, 0, 'o'},
    {"threshold", required_argument, 0, 'f'},
    {0, 0, 0, 0}
  };

  int option_index = 0, c;

  while ((c = getopt_long(argc, argv, opt_string, long_options, &option_index)) != -1) {
    switch (c) {
      case 'o':
        opt.output = optarg;
        break;
      case 'f':
        opt.threshold = atoi(optarg);
        break;
      default:
        break;
    }
  }

  /* All other argumuments are (sorted) BUS files. */
  while (optind < argc) opt.files.push_back(argv[optind++]);
  
  if (opt.files.size() == 1 && opt.files[0] == "-") {
    opt.stream_in = true;
  }
}

void parse_ProgramOptions_project(int argc, char **argv, Bustools_opt &opt) {
  
  /* Parse options. */
  const char *opt_string = "o:g:e:t:p";

  static struct option long_options[] = {
    {"output", required_argument, 0, 'o'},
    {"genemap", required_argument, 0, 'g'},
    {"ecmap", required_argument, 0, 'e'},
    {"txnames", required_argument, 0, 't'},
    {"pipe", no_argument, 0, 'p'},
    {0, 0, 0, 0}
  };

  int option_index = 0, c;

  while ((c = getopt_long(argc, argv, opt_string, long_options, &option_index)) != -1) {
    switch (c) {
      case 'o':
        opt.output = optarg;
        break;
      case 'g':
        opt.count_genes = optarg;
        break;
      case 'e':
        opt.count_ecs = optarg;
        break;
      case 't':
        opt.count_txp = optarg;
        break;
      case 'p':
        opt.stream_out = true;
        break;
      default:
        break;
    }
  }

  /* All other argumuments are (sorted) BUS files. */
  while (optind < argc) opt.files.push_back(argv[optind++]);
  
  if (opt.files.size() == 1 && opt.files[0] == "-") {
    opt.stream_in = true;
  }
}

void parse_ProgramOptions_inspect(int argc, char **argv, Bustools_opt &opt) {
  
  /* Parse options. */
  const char *opt_string = "o:e:w:p";

  static struct option long_options[] = {
    {"output", required_argument, 0, 'o'},
    {"ecmap", required_argument, 0, 'e'},
    {"whitelist", required_argument, 0, 'w'},
    {"pipe", no_argument, 0, 'p'},
    {0, 0, 0, 0}
  };

  int option_index = 0, c;

  while ((c = getopt_long(argc, argv, opt_string, long_options, &option_index)) != -1) {
    switch (c) {
      case 'o':
        opt.output = optarg;
        break;
      case 'e':
        opt.count_ecs = optarg;
        break;
      case 'w':
        opt.whitelist = optarg;
        break;
      case 'p':
        opt.stream_out = true;
        break;
      default:
        break;
    }
  }

  /* All other argumuments are (sorted) BUS files. */
  while (optind < argc) opt.files.push_back(argv[optind++]);
  
  if (opt.files.size() == 1 && opt.files[0] == "-") {
    opt.stream_in = true;
  }
}

void parse_ProgramOptions_linker(int argc, char **argv, Bustools_opt &opt) {
  
  /* Parse options. */
  const char *opt_string = "o:s:e:p:";

  static struct option long_options[] = {
    {"output", required_argument, 0, 'o'},
    {"start", required_argument, 0, 's'},
    {"end", required_argument, 0, 'e'},
    {"pipe", no_argument, 0, 'p'},
    {0, 0, 0, 0}
  };

  int option_index = 0, c;

  while ((c = getopt_long(argc, argv, opt_string, long_options, &option_index)) != -1) {
    switch (c) {
      case 'o':
        opt.output = optarg;
        break;
      case 's':
        opt.start = std::stoi(optarg);
        break;
      case 'e':
        opt.end = std::stoi(optarg);
        break;
      case 'p':
        opt.stream_out = true;
        break;
      default:
        break;
    }
  }

  /* All other argumuments are (sorted) BUS files. */
  while (optind < argc) opt.files.push_back(argv[optind++]);
  
  if (opt.files.size() == 1 && opt.files[0] == "-") {
    opt.stream_in = true;
  }
}



bool check_ProgramOptions_sort(Bustools_opt& opt) {

  bool ret = true;

  size_t max_threads = std::thread::hardware_concurrency();

  if (opt.threads <= 0) {
    std::cerr << "Error: Number of threads cannot be less than or equal to 0" << std::endl;
    ret = false;
  } else if (opt.threads > max_threads) {
    std::cerr << "Warning: Number of threads cannot be greater than or equal to " << max_threads 
    << ". Setting number of threads to " << max_threads << std::endl;
    opt.threads = max_threads;
  }

  if (!opt.stream_out && opt.output.empty()) {
    std::cerr << "Error missing output file" << std::endl;
    ret = false;
  } 

  if (opt.max_memory < 1ULL<<26) {
    if (opt.max_memory < 128) {
      std::cerr << "Warning: low number supplied for maximum memory usage with out M og G suffix\n  interpreting this as " << opt.max_memory << "Gb" << std::endl;
      opt.max_memory <<= 30;
    } else {
      std::cerr << "Warning: low number supplied for maximum memory, defaulting to 64Mb" << std::endl;
      opt.max_memory = 1ULL<<26; // 64Mb is absolute minimum
    }
  }

  if (opt.temp_files.empty()) {
    if (opt.stream_out) {
      std::cerr << "Error: temporary location -T must be set when using streaming output" << std::endl;
      ret = false;      
    } else {
      opt.temp_files = opt.output + ".";
    }
  } else {
    
    if (checkDirectoryExists(opt.temp_files)) {
      // if it is a directory, create random file prefix
      opt.temp_files += "/bus.sort." + std::to_string(getpid()) + ".";
    } else {
      int n = opt.temp_files.size();
      if (opt.temp_files[n-1] != '.') {
        opt.temp_files += '.';
      }
    }
  }

  if (opt.files.size() == 0) {
    std::cerr << "Error: Missing BUS input files" << std::endl;
    ret = false;
  } else if (!opt.stream_in) {
    for (const auto& it : opt.files) {  
      if (!checkFileExists(it)) {
        std::cerr << "Error: File not found, " << it << std::endl;
        ret = false;
      }
    }
  }

  return ret;
}


bool check_ProgramOptions_merge(Bustools_opt& opt) {
  bool ret = true;
  
  if (opt.output.empty()) {
    std::cerr << "Error missing output directory" << std::endl;
    ret = false;
  } 

  if (opt.files.size() == 0) {
    std::cerr << "Error: Missing BUS input directories" << std::endl;
    ret = false;
  } else {
    for (const auto& it : opt.files) {
      if (!checkDirectoryExists(it)) {
        std::cerr << "Error: directory " << it << " does not exist or is not a directory" << std::endl;
        ret = false;
      }
    }
    if (ret) {
      // check for output.bus and matrix.ec in each of the directories
      for (const auto &it : opt.files) {
        if (!checkFileExists(it + "/output.bus")) {
          std::cerr << "Error: file " << it << "/output.bus not found" << std::endl;
        }

        if (!checkFileExists(it + "/matrix.ec")) {
          std::cerr << "Error: file " << it << "/matrix.ec not found" << std::endl;
        }
      }
    }

    // check if output directory exists or if we can create it
    if (checkDirectoryExists(opt.output.c_str())) {
      std::cerr << "Error: file " << opt.output << " exists and is not a directory" << std::endl;
      ret = false;
    } else {
      // create directory
      if (my_mkdir(opt.output.c_str(), 0777) == -1) {
        std::cerr << "Error: could not create directory " << opt.output << std::endl;
        ret = false;
      }
    }
  }

  return ret;

}

bool check_ProgramOptions_dump(Bustools_opt& opt) {
  bool ret = true;

  if (!opt.stream_out && opt.output.empty()) {
    std::cerr << "Error missing output file" << std::endl;
    ret = false;
  } 


  if (opt.files.size() == 0) {
    std::cerr << "Error: Missing BUS input files" << std::endl;
    ret = false;
  } else if (!opt.stream_in) {    
    for (const auto& it : opt.files) {  
      if (!checkFileExists(it)) {
        std::cerr << "Error: File not found, " << it << std::endl;
        ret = false;
      }
    }
  }

  return ret;
}

bool check_ProgramOptions_capture(Bustools_opt& opt) {
  bool ret = true;

  if (!opt.stream_out && opt.output.empty()) {
    std::cerr << "Error missing output file" << std::endl;
    ret = false;
  } else if (false) { // TODO: change this to account for filter option
    // check if output directory exists or if we can create it
    if (!checkDirectoryExists(opt.output)) {
      std::cerr << "Error: file " << opt.output << " exists and is not a directory" << std::endl;
      ret = false;      
    } else {
      // create directory
      if (my_mkdir(opt.output.c_str(), 0777) == -1) {
        std::cerr << "Error: could not create directory " << opt.output << std::endl;
        ret = false;
      }
    }
  }

  if (opt.capture.empty()) {
    std::cerr << "Error: missing capture list" << std::endl;
    ret = false;
  } else {
    if (!checkFileExists(opt.capture)) {
      std::cerr << "Error: File not found, " << opt.capture << std::endl;
      ret = false;
    }
  }

  if (opt.type == TYPE_NONE) {
    std::cerr << "Error: capture list type must be specified (one of -s, -u, or -b)" << std::endl;
    ret = false;
  }

  if (opt.type == CAPTURE_TX) {
    if (opt.count_ecs.size() == 0) {
      std::cerr << "Error: missing equialence class mapping file" << std::endl;
    } else {
      if (!checkFileExists(opt.count_ecs)) {
        std::cerr << "Error: File not found " << opt.count_ecs << std::endl;
        ret = false;
      }
    }

    if (opt.count_txp.size() == 0) {
      std::cerr << "Error: missing transcript name file" << std::endl;
    } else {
      if (!checkFileExists(opt.count_txp)) {
        std::cerr << "Error: File not found " << opt.count_txp << std::endl;
        ret = false;
      }
    }
  }

  if (opt.files.size() == 0) {
    std::cerr << "Error: Missing BUS input files" << std::endl;
    ret = false;
  } else if (!opt.stream_in) {    
    for (const auto& it : opt.files) {  
      if (!checkFileExists(it)) {
        std::cerr << "Error: File not found, " << it << std::endl;
        ret = false;
      }
    }
  }

  if (opt.filter && (opt.complement || opt.type != CAPTURE_TX)) {
    std::cerr << "Warning: filter only meaningful without complement flag, and to"
      << " capture transcripts; no new ec file will be generated" << std::endl;
    opt.filter = false;
  }

  return ret;
}

bool check_ProgramOptions_correct(Bustools_opt& opt) {
  bool ret = true;

  if (!opt.stream_out && opt.output.empty()) {
    std::cerr << "Error: Missing output file" << std::endl;
    ret = false;
  } 


  if (opt.files.size() == 0) {
    std::cerr << "Error: Missing BUS input files" << std::endl;
    ret = false;
  } else {
    if (!opt.stream_in) {
      for (const auto& it : opt.files) {  
        if (!checkFileExists(it)) {
          std::cerr << "Error: File not found, " << it << std::endl;
          ret = false;
        }
      }
    }
  }

  if (opt.whitelist.size() == 0) {
    std::cerr << "Error: Missing whitelist file" << std::endl;
    ret = false;
  } else {
    if (!checkFileExists(opt.whitelist)) {
      std::cerr << "Error: File not found " << opt.whitelist << std::endl;
      ret = false;
    }
  }

  return ret;
}

bool check_ProgramOptions_count(Bustools_opt& opt) {
  bool ret = true;

  // check for output directory
  if (opt.output.empty()) {
    std::cerr << "Error: Missing output directory" << std::endl;
    ret = false;
  } else {
    if (!checkDirectoryExists(opt.output)) {
      if (checkFileExists(opt.output)) {
        std::cerr << "Error: " << opt.output << " exists and is not a directory" << std::endl;
        ret = false;
      } else if (my_mkdir(opt.output.c_str(), 0777) == -1) {
        std::cerr << "Error: could not create directory " << opt.output << std::endl;
        ret = false;
      }      
    }
  }

  

  if (opt.files.size() == 0) {
    std::cerr << "Error: Missing BUS input files" << std::endl;
    ret = false;
  } else {
    if (!opt.stream_in) {
      for (const auto& it : opt.files) {  
        if (!checkFileExists(it)) {
          std::cerr << "Error: File not found, " << it << std::endl;
          ret = false;
        }
      }
    }
  }

  if (opt.count_genes.size() == 0) {
    std::cerr << "Error: missing gene mapping file" << std::endl;
    ret = false;
  } else {
    if (!checkFileExists(opt.count_genes)) {
      std::cerr << "Error: File not found " << opt.count_genes << std::endl;
      ret = false;
    }
  }

  if (opt.count_ecs.size() == 0) {
    std::cerr << "Error: missing equialence class mapping file" << std::endl;
    ret = false;
  } else {
    if (!checkFileExists(opt.count_ecs)) {
      std::cerr << "Error: File not found " << opt.count_ecs << std::endl;
      ret = false;
    }
  }

  if (opt.count_txp.size() == 0) {
    std::cerr << "Error: missing transcript name file" << std::endl;
    ret = false;
  } else {
    if (!checkFileExists(opt.count_txp)) {
      std::cerr << "Error: File not found " << opt.count_txp << std::endl;
      ret = false;
    }
  }

  return ret;
}

bool check_ProgramOptions_whitelist(Bustools_opt &opt) {
  bool ret = true;

  if (opt.output.empty()) {
    std::cerr << "Error: Missing output file" << std::endl;
    ret = false;
  } 

  if (opt.files.size() == 0) {
    std::cerr << "Error: Missing BUS input file" << std::endl;
    ret = false;
  } else if (opt.files.size() == 1) {
    if (!opt.stream_in) {
      for (const auto& it : opt.files) {
        if (!checkFileExists(it)) {
          std::cerr << "Error: File not found, " << it << std::endl;
          ret = false;
        }
      }
    }
  } else {
    std::cerr << "Error: Only one input file allowed" << std::endl;
    ret = false;
  }

  if (opt.threshold < 0) { // threshold = 0 for no threshold
    std::cerr << "Error: Threshold cannot be less than or equal to 0" << std::endl;
    ret = false;
  }

  return ret;
}

bool check_ProgramOptions_project(Bustools_opt &opt) {
  bool ret = true;

  if (opt.output.empty()) {
    std::cerr << "Error: Missing output directory" << std::endl;
    ret = false;
  } 

  if (opt.files.size() == 0) {
    std::cerr << "Error: Missing BUS input file" << std::endl;
    ret = false;
  } else if (opt.files.size() == 1) {
    if (!opt.stream_in) {
      for (const auto& it : opt.files) {
        if (!checkFileExists(it)) {
          std::cerr << "Error: File not found, " << it << std::endl;
          ret = false;
        }
      }
    }
  } else {
    std::cerr << "Error: Only one input file allowed" << std::endl;
    ret = false;
  }

  if (opt.count_genes.size() == 0) {
    std::cerr << "Error: missing gene mapping file" << std::endl;
  } else {
    if (!checkFileExists(opt.count_genes)) {
      std::cerr << "Error: File not found " << opt.count_genes << std::endl;
      ret = false;
    }
  }
  
  if (opt.count_ecs.size() == 0) {
    std::cerr << "Error: missing equialence class mapping file" << std::endl;
  } else {
    if (!checkFileExists(opt.count_ecs)) {
      std::cerr << "Error: File not found " << opt.count_ecs << std::endl;
      ret = false;
    }
  }
  
  if (opt.count_txp.size() == 0) {
    std::cerr << "Error: missing transcript name file" << std::endl;
  } else {
    if (!checkFileExists(opt.count_genes)) {
      std::cerr << "Error: File not found " << opt.count_txp << std::endl;
      ret = false;
    }
  }

  return ret;
}

bool check_ProgramOptions_inspect(Bustools_opt &opt) {
  bool ret = true;
  
  if (opt.files.size() == 0) {
    std::cerr << "Error: Missing BUS input file" << std::endl;
    ret = false;
  } else if (opt.files.size() == 1) {
    if (!opt.stream_in) {
      for (const auto& it : opt.files) {
        if (!checkFileExists(it)) {
          std::cerr << "Error: File not found, " << it << std::endl;
          ret = false;
        }
      }
    }
  } else {
    std::cerr << "Error: Only one input file allowed" << std::endl;
    ret = false;
  }
  
  if (opt.count_ecs.size()) {
    if (!checkFileExists(opt.count_ecs)) {
      std::cerr << "Error: File not found " << opt.count_ecs << std::endl;
      ret = false;
    }
  }

  if (opt.whitelist.size()) {
    if (!checkFileExists(opt.whitelist)) {
      std::cerr << "Error: File not found " << opt.whitelist << std::endl;
      ret = false;
    }
  }
  
  return ret;
}

bool check_ProgramOptions_linker(Bustools_opt &opt) {
  bool ret = true;
  
  if (!opt.stream_out && opt.output.empty()) {
    std::cerr << "Error: Missing output file" << std::endl;
    ret = false;
  } 

  if (opt.files.size() == 0) {
    std::cerr << "Error: Missing BUS input files" << std::endl;
    ret = false;
  } else {
    if (!opt.stream_in) {
      for (const auto& it : opt.files) {  
        if (!checkFileExists(it)) {
          std::cerr << "Error: File not found, " << it << std::endl;
          ret = false;
        }
      }
    }
  }
  
  return ret;
}


void Bustools_Usage() {
  std::cout << "bustools " << BUSTOOLS_VERSION << std::endl << std::endl  
  << "Usage: bustools <CMD> [arguments] .." << std::endl << std::endl
  << "Where <CMD> can be one of: " << std::endl << std::endl
  << "capture         Capture records from a BUS file" << std::endl
  << "correct         Error correct a BUS file" << std::endl
  << "count           Generate count matrices from a BUS file" << std::endl
  << "inspect         Produce a report summarizing a BUS file" << std::endl
  << "linker          Remove section of barcodes in BUS files" << std::endl
  //<< "merge           Merge bus files from same experiment" << std::endl
  << "project         Project a BUS file to gene sets" << std::endl
  << "sort            Sort a BUS file by barcodes and UMIs" << std::endl
  << "text            Convert a binary BUS file to a tab-delimited text file" << std::endl
  << "whitelist       Generate a whitelist from a BUS file" << std::endl
  << std::endl
  << "Running bustools <CMD> without arguments prints usage information for <CMD>"
  << std::endl << std::endl;
}



void Bustools_sort_Usage() {
  std::cout << "Usage: bustools sort [options] bus-files" << std::endl << std::endl
  << "Options: " << std::endl
  << "-t, --threads         Number of threads to use" << std::endl
  << "-m, --memory          Maximum memory used" << std::endl
  << "-T, --temp            Location and prefix for temporary files " << std::endl
  << "                      required if using -p, otherwise defaults to output" << std::endl 
  << "-o, --output          File for sorted output" << std::endl
  << "-p, --pipe            Write to standard output" << std::endl
  << std::endl;
}

void Bustools_capture_Usage() {
  std::cout << "Usage: bustools capture [options] bus-files" << std::endl << std::endl
  << "Options: " << std::endl
  << "-o, --output          File for captured output " << std::endl
  << "-x, --complement      Take complement of captured set" << std::endl
  << "-c, --capture         Capture list" << std::endl
  << "-e, --ecmap           File for mapping equivalence classes to transcripts" << std::endl
  << "-t, --txnames         File with names of transcripts" << std::endl
  << "-s, --transcripts     Capture list is a list of transcripts to capture" << std::endl
  << "-u, --umis            Capture list is a list of UMIs to capture" << std::endl
  << "-b, --barcode         Capture list is a list of barcodes to capture" << std::endl
  << "-p, --pipe            Write to standard output" << std::endl
  << std::endl;
}

void Bustools_merge_Usage() {
  std::cout << "Usage: bustools merge [options] directories" << std::endl << std::endl
  << "Options: " << std::endl
  << "-t, --threads         Number of threads to use" << std::endl
  << "-o, --output          Directory for merged output" << std::endl
  << std::endl;
}

void Bustools_dump_Usage() {
  std::cout << "Usage: bustools text [options] bus-files" << std::endl << std::endl
  << "Options: " << std::endl
  << "-o, --output          File for text output" << std::endl
  << "-p, --pipe            Write to standard output" << std::endl
  << std::endl;
}

void Bustools_correct_Usage() {
  std::cout << "Usage: bustools correct [options] bus-files" << std::endl << std::endl
  << "Options: " << std::endl
  << "-o, --output          File for corrected bus output" << std::endl
  << "-w, --whitelist       File of whitelisted barcodes to correct to" << std::endl
  << "-p, --pipe            Write to standard output" << std::endl
  << std::endl;
}

void Bustools_count_Usage() {
  std::cout << "Usage: bustools count [options] sorted-bus-files" << std::endl << std::endl
  << "Options: " << std::endl
  << "-o, --output          File for corrected bus output" << std::endl
  << "-g, --genemap         File for mapping transcripts to genes" << std::endl
  << "-e, --ecmap           File for mapping equivalence classes to transcripts" << std::endl
  << "-t, --txnames         File with names of transcripts" << std::endl
  << "--genecounts          Aggregate counts to genes only" << std::endl
  << "-m, --multimapping    Include bus records that pseudoalign to multiple genes" << std::endl
  << std::endl;
}

void Bustools_whitelist_Usage() {
  std::cout << "Usage: bustools whitelist [options] sorted-bus-file" << std::endl << std::endl
    << "Options: " << std::endl
    << "-o, --output        File for the whitelist" << std::endl
    << "-f, --threshold     Minimum number of times a barcode must appear to be included in whitelist" << std::endl
    << std::endl;
}

void Bustools_project_Usage() {
  std::cout << "Usage: bustools project [options] sorted-bus-file" << std::endl << std::endl
    << "Options: " << std::endl
    << "-o, --output          File for project bug output and list of genes (no extension)" << std::endl
    << "-g, --genemap         File for mapping transcripts to genes" << std::endl
    << "-e, --ecmap           File for mapping equivalence classes to transcripts" << std::endl
    << "-t, --txnames         File with names of transcripts" << std::endl
    << "-p, --pipe            Write to standard output" << std::endl
    << std::endl;
}

void Bustools_inspect_Usage() {
  std::cout << "Usage: bustools inspect [options] sorted-bus-file" << std::endl << std::endl
    << "Options: " << std::endl
    << "-o, --output          File for JSON output (optional)" << std::endl
    << "-e, --ecmap           File for mapping equivalence classes to transcripts" << std::endl
    << "-w, --whitelist       File of whitelisted barcodes to correct to" << std::endl
    << "-p, --pipe            Write to standard output" << std::endl
    << std::endl;
}

void Bustools_linker_Usage() {
  std::cout << "Usage: bustools linker [options] bus-files" << std::endl << std::endl
    << "Options: " << std::endl
    << "-s, --start           Start coordinate for section of barcode to remove (0-indexed, inclusive)" << std::endl
    << "-e, --end             End coordinate for section of barcode to remove (0-indexed, exclusive)" << std::endl
    << "-p, --pipe            Write to standard output" << std::endl
    << std::endl;
}



int main(int argc, char **argv) {

  if (argc < 2) {
    // Print error message, function?
    Bustools_Usage();
    exit(1);
  } else {
    bool disp_help = argc == 2;
    std::string cmd(argv[1]);
    Bustools_opt opt;


    if (cmd == "sort") {
      if (disp_help) {
        Bustools_sort_Usage();
        exit(0);        
      }
      parse_ProgramOptions_sort(argc-1, argv+1, opt);
      if (check_ProgramOptions_sort(opt)) { //Program options are valid
        //bustools_sort_orig(opt);
        bustools_sort(opt);
      } else {
        Bustools_sort_Usage();
        exit(1);
      }
    } else if (cmd == "merge") {
      if (disp_help) {
        Bustools_merge_Usage();
        exit(0);
      }
      parse_ProgramOptions_merge(argc-1, argv+1, opt);
      if (check_ProgramOptions_merge(opt)) {
        // first parse all headers
        std::vector<BUSHeader> vh;
        // TODO: check for compatible headers, version numbers umi and bclen

        for (const auto& infn : opt.files) {
          std::ifstream inf((infn + "/output.bus").c_str(), std::ios::binary);
          BUSHeader h;
          parseHeader(inf, h);
          inf.close();
          
          parseECs(infn + "/matrix.ec", h);
          vh.push_back(std::move(h));
        }

        // create master ec
        BUSHeader oh;
        oh.version = BUSFORMAT_VERSION;
        oh.text = "Merged files from BUStools";
        //TODO: parse the transcripts file, check that they are identical and merge.
        oh.bclen = vh[0].bclen;
        oh.umilen = vh[0].umilen;
        std::unordered_map<std::vector<int32_t>, int32_t, SortedVectorHasher> ecmapinv;
        std::vector<std::vector<int32_t>> ectrans;        
        std::vector<int32_t> ctrans;
        
        oh.ecs = vh[0].ecs; // copy operator

        for (int32_t ec = 0; ec < oh.ecs.size(); ec++) {
          ctrans.push_back(ec);
          const auto &v = oh.ecs[ec];
          ecmapinv.insert({v, ec});
        }
        ectrans.push_back(std::move(ctrans));
        
        for (int i = 1; i < opt.files.size(); i++) {
          ctrans.clear();
          // merge the rest of the ecs
          int j = -1;
          for (const auto &v : vh[i].ecs) {
            j++;
            int32_t ec = -1;
            auto it = ecmapinv.find(v);
            if (it != ecmapinv.end()) {
              ec = it->second;              
            } else {
              ec = ecmapinv.size();
              oh.ecs.push_back(v); // copy
              ecmapinv.insert({v,ec});
            }
            ctrans.push_back(ec);
          }
          ectrans.push_back(ctrans);
        }

        // now create a single output file
        writeECs(opt.output + "/matrix.ec", oh);
        std::ofstream outf(opt.output + "/output.bus");
        writeHeader(outf, oh);


        size_t N = 100000;
        BUSData* p = new BUSData[N];
        size_t nr = 0;
        for (int i = 0; i < opt.files.size(); i++) {
          // open busfile and parse header
          BUSHeader h;
          const auto &ctrans = ectrans[i];
          std::ifstream inf((opt.files[i] + "/output.bus").c_str(), std::ios::binary);
          parseHeader(inf, h);
          // now read all records and translate the ecs
          while (true) {
            inf.read((char*)p, N*sizeof(BUSData));
            size_t rc = inf.gcount() / sizeof(BUSData);
            if (rc == 0) {
              break;
            }
            nr += rc;
            for (size_t i = 0; i < rc; i++) {
              auto &b = p[i];
              b.ec = ctrans[b.ec]; // modify the ec              
            }
            outf.write((char*)p, rc*sizeof(BUSData));
          }
          inf.close();
        }
        outf.close();
      } else {
        Bustools_merge_Usage();
        exit(1);
      }

    } else if (cmd == "dump" || cmd == "text") {
      if (disp_help) {
        Bustools_dump_Usage();
        exit(0);        
      }
      parse_ProgramOptions_dump(argc-1, argv+1, opt);
      if (check_ProgramOptions_dump(opt)) { //Program options are valid
        BUSHeader h;
        size_t nr = 0;
        size_t N = 100000;
        BUSData* p = new BUSData[N];

        std::streambuf *buf = nullptr;
        std::ofstream of;

        if (!opt.stream_out) {
          of.open(opt.output); 
          buf = of.rdbuf();
        } else {
          buf = std::cout.rdbuf();
        }
        std::ostream o(buf);


        char magic[4];      
        uint32_t version = 0;
        for (const auto& infn : opt.files) {          
          std::streambuf *inbuf;
          std::ifstream inf;
          if (!opt.stream_in) {
            inf.open(infn.c_str(), std::ios::binary);
            inbuf = inf.rdbuf();
          } else {
            inbuf = std::cin.rdbuf();
          }
          std::istream in(inbuf);


          parseHeader(in, h);
          uint32_t bclen = h.bclen;
          uint32_t umilen = h.umilen;
          int rc = 0;
          while (true) {
            in.read((char*)p, N*sizeof(BUSData));
            size_t rc = in.gcount() / sizeof(BUSData);
            if (rc == 0) {
              break;
            }
            nr += rc;
            for (size_t i = 0; i < rc; i++) {
              o << binaryToString(p[i].barcode, bclen) << "\t" << binaryToString(p[i].UMI,umilen) << "\t" << p[i].ec << "\t" << p[i].count << "\n";        
            }
          }
        }
        delete[] p; p = nullptr;
        if (!opt.stream_out) {
          of.close();
        }
        std::cerr << "Read in " << nr << " BUS records" << std::endl;
      } else {
        Bustools_dump_Usage();
        exit(1);
      }
    } else if (cmd == "correct") {      
      if (disp_help) {
        Bustools_correct_Usage();
        exit(0);        
      }
      parse_ProgramOptions_correct(argc-1, argv+1, opt);
      if (check_ProgramOptions_correct(opt)) { //Program options are valid
        
        uint32_t bclen = 0; 
        uint32_t wc_bclen = 0;
        uint32_t umilen = 0;
        BUSHeader h;
        size_t nr = 0;
        size_t N = 100000;
        BUSData* p = new BUSData[N];
        char magic[4];      
        uint32_t version = 0;
        size_t stat_white = 0;
        size_t stat_corr = 0;
        size_t stat_uncorr = 0;

        std::ifstream wf(opt.whitelist, std::ios::in);
        std::string line;
        line.reserve(100);
        std::unordered_set<uint64_t> wbc;
        wbc.reserve(100000);
        uint32_t f = 0;
        while(std::getline(wf, line)) {
          if (wc_bclen == 0) {
            wc_bclen = line.size();
          }
          uint64_t bc = stringToBinary(line, f);
          wbc.insert(bc);          
        }
        wf.close();

        std::cerr << "Found " << wbc.size() << " barcodes in the whitelist" << std::endl;

        std::unordered_map<uint64_t, uint64_t> correct;
        correct.reserve(wbc.size()*3*wc_bclen);
        // whitelisted barcodes correct to themselves
        for (uint64_t b : wbc) {
          correct.insert({b,b});
        }
        // include hamming distance 1 to all codewords
        std::vector<uint64_t> bad_y;
        for (auto x : wbc) {
          // insert all hamming distance one
          size_t sh = wc_bclen-1;          

          for (size_t i = 0; i < wc_bclen; ++i) {
            for (uint64_t d = 1; d <= 3; d++) {
              uint64_t y = x ^ (d << (2*sh));
              if (correct.find(y) != correct.end()) {
                bad_y.push_back(y);
              } else {
                correct.insert({y,x});
              }
            }                
            sh--;
          }
        }

        // paranoia about error correcting
        int removed_ys = 0;
        for (auto y : bad_y) {
          if (wbc.find(y) == wbc.end()) {
            if (correct.erase(y)>0) { // duplicates are fine
              removed_ys++;
            }
          }
        }

        std::cerr << "Number of hamming dist 1 barcodes = " << correct.size() << std::endl;
        
        std::streambuf *buf = nullptr;
        std::ofstream busf_out;
        
        if (!opt.stream_out) {
          busf_out.open(opt.output , std::ios::out | std::ios::binary);
          buf = busf_out.rdbuf();
        } else {
          buf = std::cout.rdbuf();
        }
        std::ostream bus_out(buf);

        bool outheader_written = false;
        
        nr = 0;
        BUSData bd;
        for (const auto& infn : opt.files) { 
          std::streambuf *inbuf;
          std::ifstream inf;
          if (!opt.stream_in) {
            inf.open(infn.c_str(), std::ios::binary);
            inbuf = inf.rdbuf();
          } else {
            inbuf = std::cin.rdbuf();
          }
          std::istream in(inbuf);          
          parseHeader(in, h);

          if (!outheader_written) {
            writeHeader(bus_out, h);
            outheader_written = true;
          }

          if (bclen == 0) {
            bclen = h.bclen;

            if (bclen != wc_bclen) { 
              std::cerr << "Error: barcode length and whitelist length differ, barcodes = " << bclen << ", whitelist = " << wc_bclen << std::endl
                        << "       check that your whitelist matches the technology used" << std::endl;

              exit(1);
            }
          }
          if (umilen == 0) {
            umilen = h.umilen;
          }

          int rc = 0;
          while (true) {
            in.read((char*)p, N*sizeof(BUSData));
            size_t rc = in.gcount() / sizeof(BUSData);
            if (rc == 0) {
              break;
            }
            nr +=rc;

            for (size_t i = 0; i < rc; i++) {
              bd = p[i];
              auto it = correct.find(bd.barcode);
              if (it != correct.end()) {
                if (bd.barcode != it->second) {
                  bd.barcode = it->second;
                  stat_corr++;
                } else {
                  stat_white++;
                }
                bd.count = 1;
                bus_out.write((char*) &bd, sizeof(bd));
              } else {
                stat_uncorr++;
              }
            }
          }
        }

        std::cerr << "Processed " << nr << " bus records" << std::endl
        << "In whitelist = " << stat_white << std::endl
        << "Corrected = " << stat_corr << std::endl
        << "Uncorrected = " << stat_uncorr << std::endl;


        if (!opt.stream_out) {
          busf_out.close();
        }

        delete[] p; p = nullptr;
      } else {
        Bustools_dump_Usage();
        exit(1);
      }
    } else if (cmd == "fromtext") {
      BUSHeader h;
      uint32_t f;
      bool out_header_written = false;
      std::string line, bc, umi;
      int32_t ec,count;

      while(std::getline(std::cin, line)) {
        std::stringstream ss(line);
        ss >> bc >> umi >> ec >> count;
        if (!out_header_written) {
          h.bclen = bc.size();
          h.umilen = umi.size();
          h.version = BUSFORMAT_VERSION;
          h.text = "converted from text format";
          writeHeader(std::cout, h);
          out_header_written = true;
        }
        BUSData b;
        b.barcode = stringToBinary(bc, f);
        b.UMI = stringToBinary(umi, f);
        b.ec = ec;
        b.count = count;
        b.flags = 0;
        std::cout.write((char*)&b, sizeof(b));
      }

    } else if (cmd == "count") {
      if (disp_help) {
        Bustools_count_Usage();
        exit(0);        
      }
      parse_ProgramOptions_count(argc-1, argv+1, opt);
      if (check_ProgramOptions_count(opt)) { //Program options are valid
        bustools_count(opt);
      } else {
        Bustools_count_Usage();
        exit(1);
      }
    } else if (cmd == "capture") {
      if (disp_help) {
        Bustools_capture_Usage();
        exit(0);        
      }
      parse_ProgramOptions_capture(argc-1, argv+1, opt);
      if (check_ProgramOptions_capture(opt)) { //Program options are valid
        bustools_capture(opt);
      } else {
        Bustools_dump_Usage();
        exit(1);
      }
    } else if (cmd == "whitelist") {
      if (disp_help) {
        Bustools_whitelist_Usage();
        exit(0);        
      }
      parse_ProgramOptions_whitelist(argc-1, argv+1, opt);
      if (check_ProgramOptions_whitelist(opt)) { //Program options are valid
        bustools_whitelist(opt);
      } else {
        Bustools_whitelist_Usage();
        exit(1);
      }
    } else if (cmd == "project") {
      if (disp_help) {
        Bustools_project_Usage();
        exit(0);
      }
      parse_ProgramOptions_project(argc-1, argv+1, opt);
      if (check_ProgramOptions_project(opt)) { //Program options are valid
        bustools_project(opt);
      } else {
        Bustools_project_Usage();
        exit(1);
      }
    } else if (cmd == "inspect") {
      if (disp_help) {
        Bustools_inspect_Usage();
        exit(0);
      }
      parse_ProgramOptions_inspect(argc-1, argv+1, opt);
      if (check_ProgramOptions_inspect(opt)) { //Program options are valid
        bustools_inspect(opt);
      } else {
        Bustools_inspect_Usage();
        exit(1);
      }
    } else if (cmd == "linker") {
      if (disp_help) {
        Bustools_linker_Usage();
        exit(0);
      }
      parse_ProgramOptions_linker(argc-1, argv+1, opt);
      if (check_ProgramOptions_linker(opt)) { //Program options are valid
        bustools_linker(opt);
      } else {
        Bustools_linker_Usage();
        exit(1);
      }
    } else {
      std::cerr << "Error: invalid command " << cmd << std::endl;
      Bustools_Usage();      
    }

  }
}
