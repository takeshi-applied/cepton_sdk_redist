#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cepton_sdk.h"


#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

void common_sleep(int milliseconds) {
  //sleep:
#ifdef _WIN32
  Sleep(milliseconds);
#else
  usleep(milliseconds * 1000);
#endif
}


FILE *fh = NULL;
int got_frame = 0; 
int file_mode = 0; // 0-CSV, 1-BIN
int frame_to_get = 1;

void on_frame(int error_code, CeptonSensorHandle sensor,
  size_t n_points, struct CeptonSensorPoint const *p_points) {
  if (got_frame == 0) {
    got_frame++;
    return; // Skip first partial frame
  }
  if (got_frame > frame_to_get + 1) return; // Got enough frames already
  if (file_mode) {
    fwrite(p_points, sizeof(CeptonSensorPoint), n_points, fh);
  }
  else {
    for (int i = 0; i < n_points; i++) {
      fprintf(fh, "%llu,%f,%f,%f,%f\n", (long long unsigned) p_points[i].timestamp,
        p_points[i].x, p_points[i].y, p_points[i].z,
        p_points[i].intensity);
    }
  }
  got_frame++;
}

void on_event(int error_code, CeptonSensorHandle sensor,
  struct CeptonSensorInformation const *p_info, int sensor_event) {
  switch (sensor_event) {
  case CEPTON_EVENT_ATTACH:
    cepton_sdk_listen_frames(on_frame);
    break;
  case CEPTON_EVENT_FRAME:
    break;
  }
}
int main(int argc, char **argv) {
  if (argc == 1) {
    printf(R"(
Usage: data_exporter <options> <outputfile>

Options are:
  -n <N>    Number of frames to capture, default is 1
  -f <fmt>  Valid formats are csv or bin, default is csv
      binary files uses CeptonSensorPoint for each point

)"
    );
    return 0;
  }

  int argptr = 1;
  while (argc > argptr + 1 && argv[argptr][0] == '-') {
    if (strcmp(argv[argptr], "-n") == 0) {
      frame_to_get = atoi(argv[argptr + 1]);
      if (frame_to_get <= 0 || frame_to_get > 1000) {
        printf("Invalid number of frames, maximum allowed is 1000\n");
        return -1;
      }
      argptr += 2;
    }
    else if (strcmp(argv[argptr], "-f") == 0) {
      if (strcmp(argv[argptr + 1], "csv") == 0) {
        file_mode = 0;
      }
      else if (strcmp(argv[argptr + 1], "bin") == 0) {
        file_mode = 1;
      }
      else {
        printf("Invalid format specified. Expect csv or bin\n");
        return -1;
      }
      argptr += 2;
    }
  }

  if (argc > argptr + 1) {
    printf("Extra argument after filename ignored\n");
  }
  else if (argc < argptr + 1) {
    printf("Expect a file name\n");
    return -1;
  }

  char const *fname = argv[argptr];
  if (fname[0] == '-') {
    printf("Invalid file name: %s\n", fname);
    return -1;
  }
  fh = fopen(fname, "w+b");

  int err = cepton_sdk_initialize(CEPTON_SDK_VERSION, 0, on_event);
  if (err != CEPTON_SUCCESS) {
    printf("Initialize SDK failed\n");
    return -1;
  }

  while (got_frame <= frame_to_get + 1) {
    common_sleep(200);
  }

  cepton_sdk_deinitialize();
  fclose(fh);

  return 0;
}
