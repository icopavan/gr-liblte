#ifndef PBCH_UE_WRAPPER_H
#define PBCH_UE_WRAPPER_H

#include "lte.h"
#include "plot.h"

class pbch_ue_wrapper
{
public:
  pbch_ue_wrapper();

private:
  void init_plots();
  int base_init(int frame_length);
  void base_free();
  int mib_decoder_init(int cell_id);
  int mib_decoder_run(cf_t *input, pbch_mib_t *mib);

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
  pbch_t pbch;
  lte_fft_t fft;
  chest_t chest;
  sync_t sfind, strack;
  cfo_t cfocorr;

  enum sync_state {FIND, TRACK};
};

#endif // PBCH_UE_WRAPPER_H
