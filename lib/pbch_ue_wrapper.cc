#include "pbch_ue_wrapper.h"
#include <stdio.h>
#include <stdlib.h>

#define MHZ 			    1000000
#define SAMP_FREQ 		1920000
#define FLEN 			    9600
#define FLEN_PERIOD		0.005

#define NOF_PORTS 2

pbch_ue_wrapper::pbch_ue_wrapper()
    :find_threshold(30.0),
    track_threshold(10.0),
    max_track_lost(20),
    nof_slots(-1),
    track_len(300),
    disable_plots(0)
{
}

void pbch_ue_wrapper::init_plots() {
  plot_init();
  plot_real_init(&poutfft);
  plot_real_setTitle(&poutfft, "Output FFT - Magnitude");
  plot_real_setLabels(&poutfft, "Index", "dB");
  plot_real_setYAxisScale(&poutfft, -60, 0);
  plot_real_setXAxisScale(&poutfft, 1, 504);

  plot_complex_init(&pce);
  plot_complex_setTitle(&pce, "Channel Estimates");
  plot_complex_setYAxisScale(&pce, Ip, -0.01, 0.01);
  plot_complex_setYAxisScale(&pce, Q, -0.01, 0.01);
  plot_complex_setYAxisScale(&pce, Magnitude, 0, 0.01);
  plot_complex_setYAxisScale(&pce, Phase, -M_PI, M_PI);

  plot_scatter_init(&pscatrecv);
  plot_scatter_setTitle(&pscatrecv, "Received Symbols");
  plot_scatter_setXAxisScale(&pscatrecv, -0.01, 0.01);
  plot_scatter_setYAxisScale(&pscatrecv, -0.01, 0.01);

  plot_scatter_init(&pscatequal);
  plot_scatter_setTitle(&pscatequal, "Equalized Symbols");
  plot_scatter_setXAxisScale(&pscatequal, -1, 1);
  plot_scatter_setYAxisScale(&pscatequal, -1, 1);
}

int pbch_ue_wrapper::base_init(int frame_length) {
  int i;

  if (!disable_plots) {
    init_plots();
  }

  input_buffer = (cf_t*) malloc(frame_length * sizeof(cf_t));
  if (!input_buffer) {
    perror("malloc");
    return -1;
  }

  fft_buffer = (cf_t*) malloc(CPNORM_NSYMB * 72 * sizeof(cf_t));
  if (!fft_buffer) {
    perror("malloc");
    return -1;
  }

  for (i=0;i<MAX_PORTS_CTRL;i++) {
    ce[i] = (cf_t*) malloc(CPNORM_NSYMB * 72 * sizeof(cf_t));
    if (!ce[i]) {
      perror("malloc");
      return -1;
    }
  }
  if (sync_init(&sfind, FLEN)) {
    fprintf(stderr, "Error initiating PSS/SSS\n");
    return -1;
  }
  if (sync_init(&strack, track_len)) {
    fprintf(stderr, "Error initiating PSS/SSS\n");
    return -1;
  }
  if (chest_init(&chest, LINEAR, CPNORM, 6, NOF_PORTS)) {
    fprintf(stderr, "Error initializing equalizer\n");
    return -1;
  }

  if (cfo_init(&cfocorr, FLEN)) {
    fprintf(stderr, "Error initiating CFO\n");
    return -1;
  }

  if (lte_fft_init(&fft, CPNORM, 6)) {
    fprintf(stderr, "Error initializing FFT\n");
    return -1;
  }

  return 0;
}

void pbch_ue_wrapper::base_free() {
  int i;
  plot_exit();

  sync_free(&sfind);
  sync_free(&strack);
  lte_fft_free(&fft);
  chest_free(&chest);
  cfo_free(&cfocorr);

  free(input_buffer);
  free(fft_buffer);
  for (i=0;i<MAX_PORTS_CTRL;i++) {
    free(ce[i]);
  }
}

int pbch_ue_wrapper::mib_decoder_init(int cell_id) {

  if (chest_ref_LTEDL(&chest, cell_id)) {
    fprintf(stderr, "Error initializing reference signal\n");
    return -1;
  }

  if (pbch_init(&pbch, 6, cell_id, CPNORM)) {
    fprintf(stderr, "Error initiating PBCH\n");
    return -1;
  }
  DEBUG("PBCH initiated cell_id=%d\n", cell_id);
  return 0;
}

int pbch_ue_wrapper::mib_decoder_run(cf_t *input, pbch_mib_t *mib) {
  int i, n;
  lte_fft_run(&fft, input, fft_buffer);

  /* Get channel estimates for each port */
  for (i=0;i<NOF_PORTS;i++) {
    chest_ce_slot_port(&chest, fft_buffer, ce[i], 1, i);
  }

  DEBUG("Decoding PBCH\n", 0);
  n = pbch_decode(&pbch, fft_buffer, ce, 1, mib);

  float tmp[72*7];
  if (!disable_plots) {
    for (i=0;i<72*7;i++) {
      tmp[i] = 10*log10f(cabsf(fft_buffer[i]));
    }
    plot_real_setNewData(&poutfft, tmp, 72*7);
    plot_complex_setNewData(&pce, ce[0], 72*7);
    plot_scatter_setNewData(&pscatrecv, pbch.pbch_symbols[0], pbch.nof_symbols);
    if (n) {
      plot_scatter_setNewData(&pscatequal, pbch.pbch_d, pbch.nof_symbols);
    }
  }

  return n;
}
