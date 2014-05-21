#include "pbch_ue_wrapper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    disable_plots(0),
    buffer_idx(0)
{
  base_init(FLEN);
}

pbch_ue_wrapper::~pbch_ue_wrapper()
{
  base_free();
}

void pbch_ue_wrapper::test(){

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

  sync_pss_det_peak_to_avg(&sfind);
  sync_pss_det_peak_to_avg(&strack);

  nof_found_mib = 0;
  timeoffset = 0;
  state = FIND;
  nslot = 0;
  find_idx = 0;
  cfo = 0;
  mib.sfn = -1;
  frame_cnt = 0;
  last_found = 0;
  sync_set_threshold(&sfind, find_threshold);
  sync_force_N_id_2(&sfind, -1);

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

int pbch_ue_wrapper::process(cf_t* input, int n) {
  while(n > 0) {
    if( n >= FLEN-buffer_idx) {
      memcpy(&input_buffer[buffer_idx], input, (FLEN-buffer_idx)*sizeof(cf_t));
      n -= FLEN-buffer_idx;
      input += FLEN-buffer_idx;
      process_frame();
      buffer_idx = 0;
    }else{
      memcpy(&input_buffer[buffer_idx], input, n*sizeof(cf_t));
      buffer_idx += n;
      n = 0;
    }
  }
}

int pbch_ue_wrapper::process_frame() {
  switch(state) {
  case FIND:
    /* find peak in all frame */
    find_idx = sync_run(&sfind, input_buffer);
    INFO("FIND %3d:\tPAR=%.2f\n", frame_cnt, sync_get_peak_to_avg(&sfind));
    if (find_idx != -1) {
      /* if found peak, go to track and set track threshold */
      cell_id = sync_get_cell_id(&sfind);
      if (cell_id != -1) {
        frame_cnt = -1;
        last_found = 0;
        sync_set_threshold(&strack, track_threshold);
        sync_force_N_id_2(&strack, sync_get_N_id_2(&sfind));
        sync_force_cp(&strack, sync_get_cp(&sfind));
        mib_decoder_init(cell_id);
        nof_found_mib = 0;
        nslot = sync_get_slot_id(&sfind);
        nslot=(nslot+10)%20;
        cfo = 0;
        timeoffset = 0;
        printf("\n");
        state = TRACK;
      } else {
        printf("cellid=-1\n");
      }
    }
    if (verbose == VERBOSE_NONE) {
      printf("Finding PSS... PAR=%.2f\r", sync_get_peak_to_avg(&sfind));
    }
    break;
  case TRACK:
    /* Find peak around known position find_idx */
    INFO("TRACK %3d: PSS find_idx %d offset %d\n", frame_cnt, find_idx, find_idx - track_len);
    track_idx = sync_run(&strack, &input_buffer[find_idx - track_len]);

    if (track_idx != -1) {
      /* compute cumulative moving average CFO */
      cfo = (sync_get_cfo(&strack) + frame_cnt * cfo) / (frame_cnt + 1);
      /* compute cumulative moving average time offset */
      timeoffset = (float) (track_idx-track_len + timeoffset * frame_cnt) / (frame_cnt + 1);
      last_found = frame_cnt;
      find_idx = (find_idx + track_idx - track_len)%FLEN;
      if (nslot != sync_get_slot_id(&strack)) {
        INFO("Expected slot %d but got %d\n", nslot, sync_get_slot_id(&strack));
        printf("\r\n");
        fflush(stdout);
        printf("\r\n");
        state = FIND;
      }
    } else {
      /* if sync not found, adjust time offset with the averaged value */
      find_idx = (find_idx + (int) timeoffset)%FLEN;
    }

    /* if we missed too many PSS go back to FIND */
    if (frame_cnt - last_found > max_track_lost) {
      INFO("%d frames lost. Going back to FIND", frame_cnt - last_found);
      printf("\r\n");
      fflush(stdout);
      printf("\r\n");
      state = FIND;
    }

    // Correct CFO
    INFO("Correcting CFO=%.4f\n", cfo);

    cfo_correct(&cfocorr, input_buffer, -cfo/128);

    if (nslot == 0 && find_idx + 960 < FLEN) {
      INFO("Finding MIB at idx %d\n", find_idx);
      if (mib_decoder_run(&input_buffer[find_idx], &mib)) {
        INFO("MIB detected attempt=%d\n", frame_cnt);
        if (verbose == VERBOSE_NONE) {
          if (!nof_found_mib) {
            printf("\r\n");
            fflush(stdout);
            printf("\r\n");
            printf(" - Phy. CellId:\t%d\n", cell_id);
            pbch_mib_fprint(stdout, &mib);
          }
        }
        nof_found_mib++;
      } else {
        INFO("MIB not found attempt %d\n",frame_cnt);
      }
      if (frame_cnt) {
        printf("SFN: %4d, CFO: %+.4f KHz, SFO: %+.4f Khz, TimeOffset: %4d, Errors: %4d/%4d, ErrorRate: %.1e\r", mib.sfn,
            cfo*15, timeoffset/5, find_idx, frame_cnt-2*(nof_found_mib-1), frame_cnt,
            (float) (frame_cnt-2*(nof_found_mib-1))/frame_cnt);
        fflush(stdout);
      }
    }
    nslot = (nslot+10)%20;
    break;
  }
  frame_cnt++;
}
