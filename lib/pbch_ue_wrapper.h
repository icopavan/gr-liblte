#ifndef PBCH_UE_WRAPPER_H
#define PBCH_UE_WRAPPER_H

#include "lte.h"
#include "plot.h"

enum sync_state {FIND, TRACK};

class pbch_ue_wrapper
{
public:
  pbch_ue_wrapper();
  virtual ~pbch_ue_wrapper();
  int process(cf_t* input, int n);
  void test();

private:
  void init_plots();
  int base_init(int frame_length);
  void base_free();
  int mib_decoder_init(int cell_id);
  int mib_decoder_run(cf_t *input, pbch_mib_t *mib);
  int process_frame();

  plot_real_t poutfft;
  plot_complex_t pce;
  plot_scatter_t pscatrecv, pscatequal;

  float find_threshold;
  float track_threshold;
  int max_track_lost;
  int nof_slots;
  int track_len;
  int disable_plots;

  filesource_t fsrc;
  cf_t *input_buffer, *fft_buffer, *ce[MAX_PORTS_CTRL];
  int buffer_idx;
  pbch_t pbch;
  lte_fft_t fft;
  chest_t chest;
  sync_t sfind, strack;
  cfo_t cfocorr;

  int frame_cnt;
  int cell_id;
  int find_idx, track_idx, last_found;
  enum sync_state state;
  int nslot;
  pbch_mib_t mib;
  float cfo;
  int n;
  int nof_found_mib;
  float timeoffset;
};

#endif // PBCH_UE_WRAPPER_H
