/*=============================================================================
Cyan Technology Ltd
eCOG1 C Library

FILE ecog1.h - description of hardware registers

DESCRIPTION
   The regisisters/fields are in seperate structures that are mapped into the
   correct physical location by cstartup.asm.
=============================================================================*/

/* Copyright (C) Cyan Technology Ltd, 2001 */

#ifndef __ecog1_h
#define __ecog1_h


typedef struct __rg_tag              __rg_t ;
typedef struct __duart_rg_tag        __duart_rg_t ;
typedef struct __dusart_rg_tag       __dusart_rg_t ;
typedef struct __tim_rg_tag          __tim_rg_t ;
typedef struct __cache_rg_tag        __cache_rg_t ;
typedef struct __mmu_rg_tag          __mmu_rg_t ;
typedef struct __ssm_rg_tag          __ssm_rg_t ;
typedef struct __emi_rg_tag          __emi_rg_t ;
typedef struct __intact_rg_tag       __intact_rg_t ;
typedef struct __port_rg_tag         __port_rg_t ;
typedef struct __io_rg_tag           __io_rg_t ;
typedef struct __ehi_rg_tag          __ehi_rg_t ;
typedef struct __adc_rg_tag          __adc_rg_t ;
typedef struct __flash_rg_tag        __flash_rg_t ;



struct __duart_rg_tag
{
   unsigned int ctrl ;                                         /*       FEA0 */
   unsigned int frame_cfg ;                                    /*       FEA1 */
   unsigned int a_tmr_cfg ;                                    /*       FEA2 */
   unsigned int a_baud ;                                       /*       FEA3 */
   unsigned int a_sts ;                                        /*       FEA4 */
   unsigned int a_int_en ;                                     /*       FEA5 */
   unsigned int a_int_dis ;                                    /*       FEA6 */
   unsigned int a_int_clr ;                                    /*       FEA7 */
   unsigned int a_tx8 ;                                        /*       FEA8 */
   unsigned int a_tx16 ;                                       /*       FEA9 */
   unsigned int a_rx ;                                         /*       FEAA */
   unsigned int b_tmr_cfg ;                                    /*       FEAB */
   unsigned int b_baud ;                                       /*       FEAC */
   unsigned int b_sts ;                                        /*       FEAD */
   unsigned int b_int_en ;                                     /*       FEAE */
   unsigned int b_int_dis ;                                    /*       FEAF */
   unsigned int b_int_clr ;                                    /*       FEB0 */
   unsigned int b_rx ;                                         /*       FEB1 */
   unsigned int b_tx8 ;                                        /*       FEB2 */
   unsigned int b_tx16 ;                                       /*       FEB3 */
};

struct __dusart_rg_tag
{
   unsigned int a_cfg ;                                        /*       FEB4 */
   unsigned int a_smpl_cfg ;                                   /*       FEB5 */
   unsigned int a_sym_cfg ;                                    /*       FEB6 */
   unsigned int a_tim_cfg ;                                    /*       FEB7 */
   unsigned int a0_tx8 ;                                       /*       FEB8 */
   unsigned int a0_tx16 ;                                      /*       FEB9 */
   unsigned int a0_tx8_last ;                                  /*       FEBA */
   unsigned int a0_tx16_last ;                                 /*       FEBB */
   unsigned int a1_tx8 ;                                       /*       FEBC */
   unsigned int a1_tx16 ;                                      /*       FEBD */
   unsigned int a1_tx8_last ;                                  /*       FEBE */
   unsigned int a1_tx16_last ;                                 /*       FEBF */
   unsigned int a0_rx8 ;                                       /*       FEC0 */
   unsigned int a0_rx16 ;                                      /*       FEC1 */
   unsigned int a0_rx8_last ;                                  /*       FEC2 */
   unsigned int a0_rx16_last ;                                 /*       FEC3 */
   unsigned int a1_rx8 ;                                       /*       FEC4 */
   unsigned int a1_rx16 ;                                      /*       FEC5 */
   unsigned int a1_rx8_last ;                                  /*       FEC6 */
   unsigned int a1_rx16_last ;                                 /*       FEC7 */
   unsigned int a_int_sts ;                                    /*       FEC8 */
   unsigned int a_int_en ;                                     /*       FEC9 */
   unsigned int a_int_dis ;                                    /*       FECA */
   unsigned int a_int_clr ;                                    /*       FECB */
   unsigned int a_ex_sts ;                                     /*       FECC */
   unsigned int a_ex_en ;                                      /*       FECD */
   unsigned int a_ex_dis ;                                     /*       FECE */
   unsigned int a_ex_clr ;                                     /*       FECF */
   unsigned int b_cfg ;                                        /*       FED0 */
   unsigned int b_smpl_cfg ;                                   /*       FED1 */
   unsigned int b_sym_cfg ;                                    /*       FED2 */
   unsigned int b_tim_cfg ;                                    /*       FED3 */
   unsigned int b0_tx8 ;                                       /*       FED4 */
   unsigned int b0_tx16 ;                                      /*       FED5 */
   unsigned int b0_tx8_last ;                                  /*       FED6 */
   unsigned int b0_tx16_last ;                                 /*       FED7 */
   unsigned int b1_tx8 ;                                       /*       FED8 */
   unsigned int b1_tx16 ;                                      /*       FED9 */
   unsigned int b1_tx8_last ;                                  /*       FEDA */
   unsigned int b1_tx16_last ;                                 /*       FEDB */
   unsigned int b0_rx8 ;                                       /*       FEDC */
   unsigned int b0_rx16 ;                                      /*       FEDD */
   unsigned int b0_rx8_last ;                                  /*       FEDE */
   unsigned int b0_rx16_last ;                                 /*       FEDF */
   unsigned int b1_rx8 ;                                       /*       FEE0 */
   unsigned int b1_rx16 ;                                      /*       FEE1 */
   unsigned int b1_rx8_last ;                                  /*       FEE2 */
   unsigned int b1_rx16_last ;                                 /*       FEE3 */
   unsigned int b_int_sts ;                                    /*       FEE4 */
   unsigned int b_int_en ;                                     /*       FEE5 */
   unsigned int b_int_dis ;                                    /*       FEE6 */
   unsigned int b_int_clr ;                                    /*       FEE7 */
   unsigned int b_ex_sts ;                                     /*       FEE8 */
   unsigned int b_ex_en ;                                      /*       FEE9 */
   unsigned int b_ex_dis ;                                     /*       FEEA */
   unsigned int b_ex_clr ;                                     /*       FEEB */
   unsigned int i2c_cfg ;                                      /*       FEEC */
   unsigned int i2c_slave_cfg ;                                /*       FEED */
   unsigned int i2c_master_cmd ;                               /*       FEEE */
   unsigned int spi_tx_cfg ;                                   /*       FEEF */
   unsigned int spi_rx_cfg ;                                   /*       FEF0 */
   unsigned int spi_ctrl ;                                     /*       FEF1 */
   unsigned int spi_frame_ctrl ;                               /*       FEF2 */
   unsigned int __fill0 ;                                      /*       FEF3 */
   unsigned int uart_cfg ;                                     /*       FEF4 */
   unsigned int uart_ctrl ;                                    /*       FEF5 */
   unsigned int sc_ctrl ;                                      /*       FEF6 */
   unsigned int sc_sts ;                                       /*       FEF7 */
   unsigned int sc_int_en ;                                    /*       FEF8 */
   unsigned int sc_int_dis ;                                   /*       FEF9 */
   unsigned int sc_int_clr ;                                   /*       FEFA */
   unsigned int __fill1 ;                                      /*       FEFB */
   unsigned int sc_cfg ;                                       /*       FEFC */
   unsigned int sc_tim_cfg1 ;                                  /*       FEFD */
   unsigned int sc_tim_cfg2 ;                                  /*       FEFE */
   unsigned int sc_tim_cfg3 ;                                  /*       FEFF */
   unsigned int ir_ctrl ;                                      /*       FF00 */
   unsigned int ir_sts ;                                       /*       FF01 */
   unsigned int ir_int_en ;                                    /*       FF02 */
   unsigned int ir_int_dis ;                                   /*       FF03 */
   unsigned int ir_int_clr ;                                   /*       FF04 */
   unsigned int __fill2 ;                                      /*       FF05 */
   unsigned int ir_ldin_cfg ;                                  /*       FF06 */
   unsigned int ir_d0_cfg ;                                    /*       FF07 */
   unsigned int ir_d1_cfg ;                                    /*       FF08 */
   unsigned int ir_ldout_cfg ;                                 /*       FF09 */
   unsigned int ir_thresh_cfg ;                                /*       FF0A */
   unsigned int ir_len_cfg ;                                   /*       FF0B */
   unsigned int ir_rx_cfg ;                                    /*       FF0C */
   unsigned int ir_rx_bit_cfg ;                                /*       FF0D */
   unsigned int ir_rx_d0_cfg ;                                 /*       FF0E */
   unsigned int ir_rx_d1_cfg ;                                 /*       FF0F */
   unsigned int ir_frame_cfg ;                                 /*       FF10 */
   unsigned int usr_a_en ;                                     /*       FF11 */
   unsigned int usr_a_dis ;                                    /*       FF12 */
   unsigned int usr_b_en ;                                     /*       FF13 */
   unsigned int usr_b_dis ;                                    /*       FF14 */
   unsigned int usr_a_cmd ;                                    /*       FF15 */
   unsigned int usr_b_cmd ;                                    /*       FF16 */
   unsigned int usr_a_cfg1 ;                                   /*       FF17 */
   unsigned int usr_b_cfg1 ;                                   /*       FF18 */
   unsigned int usr_a_cfg2 ;                                   /*       FF19 */
   unsigned int usr_b_cfg2 ;                                   /*       FF1A */
   unsigned int usr_a_cfg3 ;                                   /*       FF1B */
   unsigned int usr_b_cfg3 ;                                   /*       FF1C */
};

struct __tim_rg_tag
{
   unsigned int cmd ;                                          /*       FF1D */
   unsigned int ctrl_en ;                                      /*       FF1E */
   unsigned int ctrl_dis ;                                     /*       FF1F */
   unsigned int tmr_ld ;                                       /*       FF20 */
   unsigned int cnt1_ld ;                                      /*       FF21 */
   unsigned int cnt1_cmp ;                                     /*       FF22 */
   unsigned int cnt1_cfg ;                                     /*       FF23 */
   unsigned int cnt2_ld ;                                      /*       FF24 */
   unsigned int cnt2_cmp ;                                     /*       FF25 */
   unsigned int cnt2_cfg ;                                     /*       FF26 */
   unsigned int pwm1_ld ;                                      /*       FF27 */
   unsigned int pwm1_val ;                                     /*       FF28 */
   unsigned int pwm1_cfg ;                                     /*       FF29 */
   unsigned int pwm2_ld ;                                      /*       FF2A */
   unsigned int pwm2_val ;                                     /*       FF2B */
   unsigned int pwm2_cfg ;                                     /*       FF2C */
   unsigned int cap_cfg ;                                      /*       FF2D */
   unsigned int ex_ld ;                                        /*       FF2E */
   unsigned int ltmr_ld ;                                      /*       FF2F */
   unsigned int tmr_cnt ;                                      /*       FF30 */
   unsigned int cnt1_cnt ;                                     /*       FF31 */
   unsigned int cnt2_cnt ;                                     /*       FF32 */
   unsigned int cap_val1 ;                                     /*       FF33 */
   unsigned int cap_val2 ;                                     /*       FF34 */
   unsigned int cap_val3 ;                                     /*       FF35 */
   unsigned int cap_val4 ;                                     /*       FF36 */
   unsigned int cap_val5 ;                                     /*       FF37 */
   unsigned int cap_val6 ;                                     /*       FF38 */
   unsigned int int_sts1 ;                                     /*       FF39 */
   unsigned int int_sts2 ;                                     /*       FF3A */
   unsigned int int_en1 ;                                      /*       FF3B */
   unsigned int int_en2 ;                                      /*       FF3C */
   unsigned int int_dis1 ;                                     /*       FF3D */
   unsigned int int_dis2 ;                                     /*       FF3E */
   unsigned int int_clr1 ;                                     /*       FF3F */
   unsigned int int_clr2 ;                                     /*       FF40 */
};

struct __cache_rg_tag
{
   unsigned int cfg ;                                          /*       FF41 */
   unsigned int ctrl ;                                         /*       FF42 */
};

struct __mmu_rg_tag
{
   unsigned int translate_en ;                                 /*       FF43 */
   unsigned int flash_code_log ;                               /*       FF44 */
   unsigned int flash_code_phy ;                               /*       FF45 */
   unsigned int flash_code_size ;                              /*       FF46 */
   unsigned int ram_code_log ;                                 /*       FF47 */
   unsigned int ram_code_phy ;                                 /*       FF48 */
   unsigned int ram_code_size ;                                /*       FF49 */
   unsigned int ext_cs0_code_log ;                             /*       FF4A */
   unsigned int ext_cs0_code_phy ;                             /*       FF4B */
   unsigned int ext_cs0_code_size ;                            /*       FF4C */
   unsigned int ext_cs1_code_log ;                             /*       FF4D */
   unsigned int ext_cs1_code_phy ;                             /*       FF4E */
   unsigned int ext_cs1_code_size ;                            /*       FF4F */
   unsigned int flash_data_log ;                               /*       FF50 */
   unsigned int flash_data_phy ;                               /*       FF51 */
   unsigned int flash_data_size ;                              /*       FF52 */
   unsigned int ram_data0_log ;                                /*       FF53 */
   unsigned int ram_data0_phy ;                                /*       FF54 */
   unsigned int ram_data0_size ;                               /*       FF55 */
   unsigned int ram_data1_log ;                                /*       FF56 */
   unsigned int ram_data1_phy ;                                /*       FF57 */
   unsigned int ram_data1_size ;                               /*       FF58 */
   unsigned int cache0_data_log ;                              /*       FF59 */
   unsigned int cache1_data_log ;                              /*       FF5A */
   unsigned int ext_cs0_data0_log ;                            /*       FF5B */
   unsigned int ext_cs0_data0_phy ;                            /*       FF5C */
   unsigned int ext_cs0_data0_size ;                           /*       FF5D */
   unsigned int ext_cs0_data1_log ;                            /*       FF5E */
   unsigned int ext_cs0_data1_phy ;                            /*       FF5F */
   unsigned int ext_cs0_data1_size ;                           /*       FF60 */
   unsigned int ext_cs1_data0_log ;                            /*       FF61 */
   unsigned int ext_cs1_data0_phy ;                            /*       FF62 */
   unsigned int ext_cs1_data0_size ;                           /*       FF63 */
   unsigned int ext_cs1_data1_log ;                            /*       FF64 */
   unsigned int ext_cs1_data1_phy ;                            /*       FF65 */
   unsigned int ext_cs1_data1_size ;                           /*       FF66 */
   unsigned int flash_ctrl ;                                   /*       FF67 */
   unsigned int ram_ctrl ;                                     /*       FF68 */
   unsigned int adr_err ;                                      /*       FF69 */
};

struct __ssm_rg_tag
{
   unsigned int rst_set ;                                      /*       FF6A */
   unsigned int rst_clr ;                                      /*       FF6B */
   unsigned int clk_en ;                                       /*       FF6C */
   unsigned int clk_dis ;                                      /*       FF6D */
   unsigned int clk_deact ;                                    /*       FF6E */
   unsigned int clk_sleep_dis ;                                /*       FF6F */
   unsigned int clk_wake_en ;                                  /*       FF70 */
   unsigned int __fill0 ;                                      /*       FF71 */
   unsigned int cpu ;                                          /*       FF72 */
   unsigned int sts ;                                          /*       FF73 */
   unsigned int cfg ;                                          /*       FF74 */
   unsigned int div_sel ;                                      /*       FF75 */
   unsigned int tap_sel1 ;                                     /*       FF76 */
   unsigned int tap_sel2 ;                                     /*       FF77 */
   unsigned int tap_sel3 ;                                     /*       FF78 */
   unsigned int ex_ctrl ;                                      /*       FF79 */
};

struct __emi_rg_tag
{
   unsigned int ctrl_sts ;                                     /*       FF7A */
   unsigned int bus_cfg1 ;                                     /*       FF7B */
   unsigned int bus_cfg2 ;                                     /*       FF7C */
   unsigned int sdram_cfg ;                                    /*       FF7D */
   unsigned int sdram_cust_adr ;                               /*       FF7E */
   unsigned int sdram_cust_cmd ;                               /*       FF7F */
   unsigned int sdram_refr_per ;                               /*       FF80 */
   unsigned int sdram_refr_cnt ;                               /*       FF81 */
};

struct __intact_rg_tag
{
   unsigned int cfg ;                                          /*       FF82 */
   unsigned int cpu_cfg ;                                      /*       FF83 */
   unsigned int dma_cfg ;                                      /*       FF84 */
   unsigned int str_cfg ;                                      /*       FF85 */
   unsigned int cpu_rx_cfg ;                                   /*       FF86 */
   unsigned int dma_rx_cfg ;                                   /*       FF87 */
   unsigned int str_rx_cfg ;                                   /*       FF88 */
   unsigned int cpu_tx_cfg ;                                   /*       FF89 */
   unsigned int dma_tx_cfg ;                                   /*       FF8A */
   unsigned int str_tx_cfg ;                                   /*       FF8B */
   unsigned int ctrl ;                                         /*       FF8C */
   unsigned int clk_ctrl ;                                     /*       FF8D */
   unsigned int sts ;                                          /*       FF8E */
   unsigned int up_clk ;                                       /*       FF8F */
   unsigned int down_clk ;                                     /*       FF90 */
   unsigned int int_sts ;                                      /*       FF91 */
   unsigned int int_en ;                                       /*       FF92 */
   unsigned int __fill0 ;                                      /*       FF93 */
   unsigned int int_dis ;                                      /*       FF94 */
   unsigned int int_clr ;                                      /*       FF95 */
   unsigned int tx ;                                           /*       FF96 */
   unsigned int rx ;                                           /*       FF97 */
   unsigned int xfr_cfg ;                                      /*       FF98 */
   unsigned int xfr_ctrl ;                                     /*       FF99 */
   unsigned int xfr_sts ;                                      /*       FF9A */
};

struct __port_rg_tag
{
   unsigned int ver ;                                          /*       FF9B */
   unsigned int sel1 ;                                         /*       FF9C */
   unsigned int sel2 ;                                         /*       FF9D */
   unsigned int en ;                                           /*       FF9E */
   unsigned int dis ;                                          /*       FF9F */
};

struct __io_rg_tag
{
   unsigned int p_cfg ;                                        /*       FFA0 */
   unsigned int pa_out ;                                       /*       FFA1 */
   unsigned int pb_out ;                                       /*       FFA2 */
   unsigned int pa_in ;                                        /*       FFA3 */
   unsigned int pb_in ;                                        /*       FFA4 */
   unsigned int gp0_3_cfg ;                                    /*       FFA5 */
   unsigned int gp4_7_cfg ;                                    /*       FFA6 */
   unsigned int gp8_11_cfg ;                                   /*       FFA7 */
   unsigned int gp12_15_cfg ;                                  /*       FFA8 */
   unsigned int gp16_19_cfg ;                                  /*       FFA9 */
   unsigned int gp20_23_cfg ;                                  /*       FFAA */
   unsigned int gp24_27_cfg ;                                  /*       FFAB */
   unsigned int gp28_cfg ;                                     /*       FFAC */
   unsigned int gp0_3_out ;                                    /*       FFAD */
   unsigned int gp4_7_out ;                                    /*       FFAE */
   unsigned int gp8_11_out ;                                   /*       FFAF */
   unsigned int gp12_15_out ;                                  /*       FFB0 */
   unsigned int gp16_19_out ;                                  /*       FFB1 */
   unsigned int gp20_23_out ;                                  /*       FFB2 */
   unsigned int gp24_27_out ;                                  /*       FFB3 */
   unsigned int gp28_out ;                                     /*       FFB4 */
   unsigned int gp0_7_sts ;                                    /*       FFB5 */
   unsigned int gp8_15_sts ;                                   /*       FFB6 */
   unsigned int gp16_23_sts ;                                  /*       FFB7 */
   unsigned int gp24_28_sts ;                                  /*       FFB8 */
};

struct __ehi_rg_tag
{
   unsigned int cfg ;                                          /*       FFB9 */
   unsigned int ctrl_sts ;                                     /*       FFBA */
   unsigned int mmp_ram_phy ;                                  /*       FFBB */
   unsigned int mmp_hist ;                                     /*       FFBC */
   unsigned int dma_cfg ;                                      /*       FFBD */
   unsigned int dma_ctrl ;                                     /*       FFBE */
   unsigned int dma_xfr ;                                      /*       FFBF */
   unsigned int int_sts ;                                      /*       FFC0 */
   unsigned int int_en ;                                       /*       FFC1 */
   unsigned int int_dis ;                                      /*       FFC2 */
   unsigned int int_clr ;                                      /*       FFC3 */
};

struct __adc_rg_tag
{
   unsigned int sts ;                                          /*       FFC4 */
   unsigned int __fill0 ;                                      /*       FFC5 */
   unsigned int cfg ;                                          /*       FFC6 */
   unsigned int ctrl ;                                         /*       FFC7 */
};

struct __flash_rg_tag
{
   unsigned int prg_cfg ;                                      /*       FFC8 */
   unsigned int prg_ctrl ;                                     /*       FFC9 */
   unsigned int prg_adr ;                                      /*       FFCA */
   unsigned int prg_data ;                                     /*       FFCB */
   unsigned int inf_rd_adr ;                                   /*       FFCC */
   unsigned int inf_rd_data ;                                  /*       FFCD */
   unsigned int sect_wr_prot ;                                 /*       FFCE */
   unsigned int sect_rd_prot ;                                 /*       FFCF */
};




typedef struct __fd_tag              __fd_t ;
typedef struct __duart_fd_tag        __duart_fd_t ;
typedef struct __dusart_fd_tag       __dusart_fd_t ;
typedef struct __tim_fd_tag          __tim_fd_t ;
typedef struct __cache_fd_tag        __cache_fd_t ;
typedef struct __mmu_fd_tag          __mmu_fd_t ;
typedef struct __ssm_fd_tag          __ssm_fd_t ;
typedef struct __emi_fd_tag          __emi_fd_t ;
typedef struct __intact_fd_tag       __intact_fd_t ;
typedef struct __port_fd_tag         __port_fd_t ;
typedef struct __io_fd_tag           __io_fd_t ;
typedef struct __ehi_fd_tag          __ehi_fd_t ;
typedef struct __adc_fd_tag          __adc_fd_t ;
typedef struct __flash_fd_tag        __flash_fd_t ;



struct __duart_fd_tag
{
   struct
   {
      unsigned int   __fill0                           :  4 ;  /*    [15:12] */
      unsigned int   b_tx_brk_clr                      :  1 ;  /*       [11] */
      unsigned int   b_tx_brk_set                      :  1 ;  /*       [10] */
      unsigned int   b_rx_dis                          :  1 ;  /*        [9] */
      unsigned int   b_rx_en                           :  1 ;  /*        [8] */
      unsigned int   b_tx_dis                          :  1 ;  /*        [7] */
      unsigned int   b_tx_en                           :  1 ;  /*        [6] */
      unsigned int   a_tx_brk_clr                      :  1 ;  /*        [5] */
      unsigned int   a_tx_brk_set                      :  1 ;  /*        [4] */
      unsigned int   a_rx_dis                          :  1 ;  /*        [3] */
      unsigned int   a_rx_en                           :  1 ;  /*        [2] */
      unsigned int   a_tx_dis                          :  1 ;  /*        [1] */
      unsigned int   a_tx_en                           :  1 ;  /*        [0] */
   } ctrl ;

#define DUART_CTRL_A_TX_EN_MASK                                          0x0001
#define DUART_CTRL_A_TX_DIS_MASK                                         0x0002
#define DUART_CTRL_A_RX_EN_MASK                                          0x0004
#define DUART_CTRL_A_RX_DIS_MASK                                         0x0008
#define DUART_CTRL_A_TX_BRK_SET_MASK                                     0x0010
#define DUART_CTRL_A_TX_BRK_CLR_MASK                                     0x0020
#define DUART_CTRL_B_TX_EN_MASK                                          0x0040
#define DUART_CTRL_B_TX_DIS_MASK                                         0x0080
#define DUART_CTRL_B_RX_EN_MASK                                          0x0100
#define DUART_CTRL_B_RX_DIS_MASK                                         0x0200
#define DUART_CTRL_B_TX_BRK_SET_MASK                                     0x0400
#define DUART_CTRL_B_TX_BRK_CLR_MASK                                     0x0800

   struct
   {
      unsigned int   b_tx_pol                          :  1 ;  /*       [15] */
      unsigned int   b_rx_pol                          :  1 ;  /*       [14] */
      unsigned int   b_stop_bits                       :  2 ;  /*    [13:12] */
      unsigned int   b_parity                          :  2 ;  /*    [11:10] */
      unsigned int   b_data_size                       :  2 ;  /*      [9:8] */
      unsigned int   a_tx_pol                          :  1 ;  /*        [7] */
      unsigned int   a_rx_pol                          :  1 ;  /*        [6] */
      unsigned int   a_stop_bits                       :  2 ;  /*      [5:4] */
      unsigned int   a_parity                          :  2 ;  /*      [3:2] */
      unsigned int   a_data_size                       :  2 ;  /*      [1:0] */
   } frame_cfg ;

#define DUART_FRAME_CFG_A_DATA_SIZE_FIVE_BITS                  3
#define DUART_FRAME_CFG_A_DATA_SIZE_SIX_BITS                   2
#define DUART_FRAME_CFG_A_DATA_SIZE_SEVEN_BITS                 1
#define DUART_FRAME_CFG_A_DATA_SIZE_EIGHT_BITS                 0

#define DUART_FRAME_CFG_A_PARITY_ODD                           3
#define DUART_FRAME_CFG_A_PARITY_EVEN                          1
#define DUART_FRAME_CFG_A_PARITY_NONE                          0

#define DUART_FRAME_CFG_A_STOP_BITS_TWO                        2
#define DUART_FRAME_CFG_A_STOP_BITS_ONE_AND_HALF               1
#define DUART_FRAME_CFG_A_STOP_BITS_ONE                        0

#define DUART_FRAME_CFG_A_RX_POL_NORMAL                        1
#define DUART_FRAME_CFG_A_RX_POL_INVERTED                      0

#define DUART_FRAME_CFG_A_TX_POL_NORMAL                        1
#define DUART_FRAME_CFG_A_TX_POL_INVERTED                      0

#define DUART_FRAME_CFG_B_DATA_SIZE_FIVE_BITS                  3
#define DUART_FRAME_CFG_B_DATA_SIZE_SIX_BITS                   2
#define DUART_FRAME_CFG_B_DATA_SIZE_SEVEN_BITS                 1
#define DUART_FRAME_CFG_B_DATA_SIZE_EIGHT_BITS                 0

#define DUART_FRAME_CFG_B_PARITY_ODD                           3
#define DUART_FRAME_CFG_B_PARITY_EVEN                          1
#define DUART_FRAME_CFG_B_PARITY_NONE                          0

#define DUART_FRAME_CFG_B_STOP_BITS_TWO                        2
#define DUART_FRAME_CFG_B_STOP_BITS_ONE_AND_HALF               1
#define DUART_FRAME_CFG_B_STOP_BITS_ONE                        0

#define DUART_FRAME_CFG_B_RX_POL_NORMAL                        1
#define DUART_FRAME_CFG_B_RX_POL_INVERTED                      0

#define DUART_FRAME_CFG_B_TX_POL_NORMAL                        1
#define DUART_FRAME_CFG_B_TX_POL_INVERTED                      0

#define DUART_FRAME_CFG_A_DATA_SIZE_MASK                                 0x0003
#define DUART_FRAME_CFG_A_PARITY_MASK                                    0x000C
#define DUART_FRAME_CFG_A_STOP_BITS_MASK                                 0x0030
#define DUART_FRAME_CFG_A_RX_POL_MASK                                    0x0040
#define DUART_FRAME_CFG_A_TX_POL_MASK                                    0x0080
#define DUART_FRAME_CFG_B_DATA_SIZE_MASK                                 0x0300
#define DUART_FRAME_CFG_B_PARITY_MASK                                    0x0C00
#define DUART_FRAME_CFG_B_STOP_BITS_MASK                                 0x3000
#define DUART_FRAME_CFG_B_RX_POL_MASK                                    0x4000
#define DUART_FRAME_CFG_B_TX_POL_MASK                                    0x8000

   struct
   {
      unsigned int   __fill0                           :  4 ;  /*    [15:12] */
      unsigned int   tmo                               :  6 ;  /*     [11:6] */
      unsigned int   guard                             :  6 ;  /*      [5:0] */
   } a_tmr_cfg ;

#define DUART_A_TMR_CFG_GUARD_MASK                                       0x003F
#define DUART_A_TMR_CFG_TMO_MASK                                         0x0FC0

   struct
   {
      unsigned int   a_baud                            : 16 ;  /*     [15:0] */
   } a_baud ;

#define DUART_A_BAUD_A_BAUD_MASK                                         0xFFFF

   struct
   {
      unsigned int   __fill0                           :  2 ;  /*    [15:14] */
      unsigned int   rx_act                            :  1 ;  /*       [13] */
      unsigned int   rx_ufl                            :  1 ;  /*       [12] */
      unsigned int   rx_ofl                            :  1 ;  /*       [11] */
      unsigned int   rx_frame_err                      :  1 ;  /*       [10] */
      unsigned int   rx_2b_perr                        :  1 ;  /*        [9] */
      unsigned int   rx_1b_perr                        :  1 ;  /*        [8] */
      unsigned int   rx_perr                           :  1 ;  /*        [7] */
      unsigned int   rx_tmo                            :  1 ;  /*        [6] */
      unsigned int   rx_brk                            :  1 ;  /*        [5] */
      unsigned int   rx_2b_rdy                         :  1 ;  /*        [4] */
      unsigned int   rx_1b_rdy                         :  1 ;  /*        [3] */
      unsigned int   tx_ofl                            :  1 ;  /*        [2] */
      unsigned int   tx_act                            :  1 ;  /*        [1] */
      unsigned int   tx_rdy                            :  1 ;  /*        [0] */
   } a_sts ;

#define DUART_A_STS_TX_RDY_MASK                                          0x0001
#define DUART_A_STS_TX_ACT_MASK                                          0x0002
#define DUART_A_STS_TX_OFL_MASK                                          0x0004
#define DUART_A_STS_RX_1B_RDY_MASK                                       0x0008
#define DUART_A_STS_RX_2B_RDY_MASK                                       0x0010
#define DUART_A_STS_RX_BRK_MASK                                          0x0020
#define DUART_A_STS_RX_TMO_MASK                                          0x0040
#define DUART_A_STS_RX_PERR_MASK                                         0x0080
#define DUART_A_STS_RX_1B_PERR_MASK                                      0x0100
#define DUART_A_STS_RX_2B_PERR_MASK                                      0x0200
#define DUART_A_STS_RX_FRAME_ERR_MASK                                    0x0400
#define DUART_A_STS_RX_OFL_MASK                                          0x0800
#define DUART_A_STS_RX_UFL_MASK                                          0x1000
#define DUART_A_STS_RX_ACT_MASK                                          0x2000

   struct
   {
      unsigned int   __fill0                           :  3 ;  /*    [15:13] */
      unsigned int   rx_ufl                            :  1 ;  /*       [12] */
      unsigned int   rx_ofl                            :  1 ;  /*       [11] */
      unsigned int   rx_frame_err                      :  1 ;  /*       [10] */
      unsigned int   __fill1                           :  2 ;  /*      [9:8] */
      unsigned int   rx_perr                           :  1 ;  /*        [7] */
      unsigned int   rx_tmo                            :  1 ;  /*        [6] */
      unsigned int   rx_brk                            :  1 ;  /*        [5] */
      unsigned int   rx_2b_rdy                         :  1 ;  /*        [4] */
      unsigned int   rx_1b_rdy                         :  1 ;  /*        [3] */
      unsigned int   tx_ofl                            :  1 ;  /*        [2] */
      unsigned int   __fill2                           :  1 ;  /*        [1] */
      unsigned int   tx_rdy                            :  1 ;  /*        [0] */
   } a_int_en ;

#define DUART_A_INT_EN_TX_RDY_MASK                                       0x0001
#define DUART_A_INT_EN_TX_OFL_MASK                                       0x0004
#define DUART_A_INT_EN_RX_1B_RDY_MASK                                    0x0008
#define DUART_A_INT_EN_RX_2B_RDY_MASK                                    0x0010
#define DUART_A_INT_EN_RX_BRK_MASK                                       0x0020
#define DUART_A_INT_EN_RX_TMO_MASK                                       0x0040
#define DUART_A_INT_EN_RX_PERR_MASK                                      0x0080
#define DUART_A_INT_EN_RX_FRAME_ERR_MASK                                 0x0400
#define DUART_A_INT_EN_RX_OFL_MASK                                       0x0800
#define DUART_A_INT_EN_RX_UFL_MASK                                       0x1000

   struct
   {
      unsigned int   __fill0                           :  3 ;  /*    [15:13] */
      unsigned int   rx_ufl                            :  1 ;  /*       [12] */
      unsigned int   rx_ofl                            :  1 ;  /*       [11] */
      unsigned int   rx_frame_err                      :  1 ;  /*       [10] */
      unsigned int   __fill1                           :  2 ;  /*      [9:8] */
      unsigned int   rx_perr                           :  1 ;  /*        [7] */
      unsigned int   rx_tmo                            :  1 ;  /*        [6] */
      unsigned int   rx_brk                            :  1 ;  /*        [5] */
      unsigned int   rx_2b_rdy                         :  1 ;  /*        [4] */
      unsigned int   rx_1b_rdy                         :  1 ;  /*        [3] */
      unsigned int   tx_ofl                            :  1 ;  /*        [2] */
      unsigned int   __fill2                           :  1 ;  /*        [1] */
      unsigned int   tx_rdy                            :  1 ;  /*        [0] */
   } a_int_dis ;

#define DUART_A_INT_DIS_TX_RDY_MASK                                      0x0001
#define DUART_A_INT_DIS_TX_OFL_MASK                                      0x0004
#define DUART_A_INT_DIS_RX_1B_RDY_MASK                                   0x0008
#define DUART_A_INT_DIS_RX_2B_RDY_MASK                                   0x0010
#define DUART_A_INT_DIS_RX_BRK_MASK                                      0x0020
#define DUART_A_INT_DIS_RX_TMO_MASK                                      0x0040
#define DUART_A_INT_DIS_RX_PERR_MASK                                     0x0080
#define DUART_A_INT_DIS_RX_FRAME_ERR_MASK                                0x0400
#define DUART_A_INT_DIS_RX_OFL_MASK                                      0x0800
#define DUART_A_INT_DIS_RX_UFL_MASK                                      0x1000

   struct
   {
      unsigned int   __fill0                           :  3 ;  /*    [15:13] */
      unsigned int   rx_ufl                            :  1 ;  /*       [12] */
      unsigned int   rx_ofl                            :  1 ;  /*       [11] */
      unsigned int   rx_frame_err                      :  1 ;  /*       [10] */
      unsigned int   __fill1                           :  2 ;  /*      [9:8] */
      unsigned int   rx_perr                           :  1 ;  /*        [7] */
      unsigned int   rx_tmo                            :  1 ;  /*        [6] */
      unsigned int   rx_brk                            :  1 ;  /*        [5] */
      unsigned int   rx_2b_rdy                         :  1 ;  /*        [4] */
      unsigned int   rx_1b_rdy                         :  1 ;  /*        [3] */
      unsigned int   tx_ofl                            :  1 ;  /*        [2] */
      unsigned int   __fill2                           :  1 ;  /*        [1] */
      unsigned int   tx_rdy                            :  1 ;  /*        [0] */
   } a_int_clr ;

#define DUART_A_INT_CLR_TX_RDY_MASK                                      0x0001
#define DUART_A_INT_CLR_TX_OFL_MASK                                      0x0004
#define DUART_A_INT_CLR_RX_1B_RDY_MASK                                   0x0008
#define DUART_A_INT_CLR_RX_2B_RDY_MASK                                   0x0010
#define DUART_A_INT_CLR_RX_BRK_MASK                                      0x0020
#define DUART_A_INT_CLR_RX_TMO_MASK                                      0x0040
#define DUART_A_INT_CLR_RX_PERR_MASK                                     0x0080
#define DUART_A_INT_CLR_RX_FRAME_ERR_MASK                                0x0400
#define DUART_A_INT_CLR_RX_OFL_MASK                                      0x0800
#define DUART_A_INT_CLR_RX_UFL_MASK                                      0x1000

   struct
   {
      unsigned int   __fill0                           :  8 ;  /*     [15:8] */
      unsigned int   data                              :  8 ;  /*      [7:0] */
   } a_tx8 ;

#define DUART_A_TX8_DATA_MASK                                            0x00FF

   struct
   {
      unsigned int   data                              : 16 ;  /*     [15:0] */
   } a_tx16 ;

#define DUART_A_TX16_DATA_MASK                                           0xFFFF

   struct
   {
      unsigned int   data_2byte                        :  8 ;  /*     [15:8] */
      unsigned int   data_1byte                        :  8 ;  /*      [7:0] */
   } a_rx ;

#define DUART_A_RX_DATA_1BYTE_MASK                                       0x00FF
#define DUART_A_RX_DATA_2BYTE_MASK                                       0xFF00

   struct
   {
      unsigned int   __fill0                           :  4 ;  /*    [15:12] */
      unsigned int   tmo                               :  6 ;  /*     [11:6] */
      unsigned int   guard                             :  6 ;  /*      [5:0] */
   } b_tmr_cfg ;

#define DUART_B_TMR_CFG_GUARD_MASK                                       0x003F
#define DUART_B_TMR_CFG_TMO_MASK                                         0x0FC0

   struct
   {
      unsigned int   b_baud                            : 16 ;  /*     [15:0] */
   } b_baud ;

#define DUART_B_BAUD_B_BAUD_MASK                                         0xFFFF

   struct
   {
      unsigned int   __fill0                           :  2 ;  /*    [15:14] */
      unsigned int   rx_act                            :  1 ;  /*       [13] */
      unsigned int   rx_ufl                            :  1 ;  /*       [12] */
      unsigned int   rx_ofl                            :  1 ;  /*       [11] */
      unsigned int   rx_frame_err                      :  1 ;  /*       [10] */
      unsigned int   rx_2b_perr                        :  1 ;  /*        [9] */
      unsigned int   rx_1b_perr                        :  1 ;  /*        [8] */
      unsigned int   rx_perr                           :  1 ;  /*        [7] */
      unsigned int   rx_tmo                            :  1 ;  /*        [6] */
      unsigned int   rx_brk                            :  1 ;  /*        [5] */
      unsigned int   rx_2b_rdy                         :  1 ;  /*        [4] */
      unsigned int   rx_1b_rdy                         :  1 ;  /*        [3] */
      unsigned int   tx_ofl                            :  1 ;  /*        [2] */
      unsigned int   tx_act                            :  1 ;  /*        [1] */
      unsigned int   tx_rdy                            :  1 ;  /*        [0] */
   } b_sts ;

#define DUART_B_STS_TX_RDY_MASK                                          0x0001
#define DUART_B_STS_TX_ACT_MASK                                          0x0002
#define DUART_B_STS_TX_OFL_MASK                                          0x0004
#define DUART_B_STS_RX_1B_RDY_MASK                                       0x0008
#define DUART_B_STS_RX_2B_RDY_MASK                                       0x0010
#define DUART_B_STS_RX_BRK_MASK                                          0x0020
#define DUART_B_STS_RX_TMO_MASK                                          0x0040
#define DUART_B_STS_RX_PERR_MASK                                         0x0080
#define DUART_B_STS_RX_1B_PERR_MASK                                      0x0100
#define DUART_B_STS_RX_2B_PERR_MASK                                      0x0200
#define DUART_B_STS_RX_FRAME_ERR_MASK                                    0x0400
#define DUART_B_STS_RX_OFL_MASK                                          0x0800
#define DUART_B_STS_RX_UFL_MASK                                          0x1000
#define DUART_B_STS_RX_ACT_MASK                                          0x2000

   struct
   {
      unsigned int   __fill0                           :  3 ;  /*    [15:13] */
      unsigned int   rx_ufl                            :  1 ;  /*       [12] */
      unsigned int   rx_ofl                            :  1 ;  /*       [11] */
      unsigned int   rx_frame_err                      :  1 ;  /*       [10] */
      unsigned int   __fill1                           :  2 ;  /*      [9:8] */
      unsigned int   rx_perr                           :  1 ;  /*        [7] */
      unsigned int   rx_tmo                            :  1 ;  /*        [6] */
      unsigned int   rx_brk                            :  1 ;  /*        [5] */
      unsigned int   rx_2b_rdy                         :  1 ;  /*        [4] */
      unsigned int   rx_1b_rdy                         :  1 ;  /*        [3] */
      unsigned int   tx_ofl                            :  1 ;  /*        [2] */
      unsigned int   __fill2                           :  1 ;  /*        [1] */
      unsigned int   tx_rdy                            :  1 ;  /*        [0] */
   } b_int_en ;

#define DUART_B_INT_EN_TX_RDY_MASK                                       0x0001
#define DUART_B_INT_EN_TX_OFL_MASK                                       0x0004
#define DUART_B_INT_EN_RX_1B_RDY_MASK                                    0x0008
#define DUART_B_INT_EN_RX_2B_RDY_MASK                                    0x0010
#define DUART_B_INT_EN_RX_BRK_MASK                                       0x0020
#define DUART_B_INT_EN_RX_TMO_MASK                                       0x0040
#define DUART_B_INT_EN_RX_PERR_MASK                                      0x0080
#define DUART_B_INT_EN_RX_FRAME_ERR_MASK                                 0x0400
#define DUART_B_INT_EN_RX_OFL_MASK                                       0x0800
#define DUART_B_INT_EN_RX_UFL_MASK                                       0x1000

   struct
   {
      unsigned int   __fill0                           :  3 ;  /*    [15:13] */
      unsigned int   rx_ufl                            :  1 ;  /*       [12] */
      unsigned int   rx_ofl                            :  1 ;  /*       [11] */
      unsigned int   rx_frame_err                      :  1 ;  /*       [10] */
      unsigned int   __fill1                           :  2 ;  /*      [9:8] */
      unsigned int   rx_perr                           :  1 ;  /*        [7] */
      unsigned int   rx_tmo                            :  1 ;  /*        [6] */
      unsigned int   rx_brk                            :  1 ;  /*        [5] */
      unsigned int   rx_2b_rdy                         :  1 ;  /*        [4] */
      unsigned int   rx_1b_rdy                         :  1 ;  /*        [3] */
      unsigned int   tx_ofl                            :  1 ;  /*        [2] */
      unsigned int   __fill2                           :  1 ;  /*        [1] */
      unsigned int   tx_rdy                            :  1 ;  /*        [0] */
   } b_int_dis ;

#define DUART_B_INT_DIS_TX_RDY_MASK                                      0x0001
#define DUART_B_INT_DIS_TX_OFL_MASK                                      0x0004
#define DUART_B_INT_DIS_RX_1B_RDY_MASK                                   0x0008
#define DUART_B_INT_DIS_RX_2B_RDY_MASK                                   0x0010
#define DUART_B_INT_DIS_RX_BRK_MASK                                      0x0020
#define DUART_B_INT_DIS_RX_TMO_MASK                                      0x0040
#define DUART_B_INT_DIS_RX_PERR_MASK                                     0x0080
#define DUART_B_INT_DIS_RX_FRAME_ERR_MASK                                0x0400
#define DUART_B_INT_DIS_RX_OFL_MASK                                      0x0800
#define DUART_B_INT_DIS_RX_UFL_MASK                                      0x1000

   struct
   {
      unsigned int   __fill0                           :  3 ;  /*    [15:13] */
      unsigned int   rx_ufl                            :  1 ;  /*       [12] */
      unsigned int   rx_ofl                            :  1 ;  /*       [11] */
      unsigned int   rx_frame_err                      :  1 ;  /*       [10] */
      unsigned int   __fill1                           :  2 ;  /*      [9:8] */
      unsigned int   rx_perr                           :  1 ;  /*        [7] */
      unsigned int   rx_tmo                            :  1 ;  /*        [6] */
      unsigned int   rx_brk                            :  1 ;  /*        [5] */
      unsigned int   rx_2b_rdy                         :  1 ;  /*        [4] */
      unsigned int   rx_1b_rdy                         :  1 ;  /*        [3] */
      unsigned int   tx_ofl                            :  1 ;  /*        [2] */
      unsigned int   __fill2                           :  1 ;  /*        [1] */
      unsigned int   tx_rdy                            :  1 ;  /*        [0] */
   } b_int_clr ;

#define DUART_B_INT_CLR_TX_RDY_MASK                                      0x0001
#define DUART_B_INT_CLR_TX_OFL_MASK                                      0x0004
#define DUART_B_INT_CLR_RX_1B_RDY_MASK                                   0x0008
#define DUART_B_INT_CLR_RX_2B_RDY_MASK                                   0x0010
#define DUART_B_INT_CLR_RX_BRK_MASK                                      0x0020
#define DUART_B_INT_CLR_RX_TMO_MASK                                      0x0040
#define DUART_B_INT_CLR_RX_PERR_MASK                                     0x0080
#define DUART_B_INT_CLR_RX_FRAME_ERR_MASK                                0x0400
#define DUART_B_INT_CLR_RX_OFL_MASK                                      0x0800
#define DUART_B_INT_CLR_RX_UFL_MASK                                      0x1000

   struct
   {
      unsigned int   data_2byte                        :  8 ;  /*     [15:8] */
      unsigned int   data_1byte                        :  8 ;  /*      [7:0] */
   } b_rx ;

#define DUART_B_RX_DATA_1BYTE_MASK                                       0x00FF
#define DUART_B_RX_DATA_2BYTE_MASK                                       0xFF00

   struct
   {
      unsigned int   __fill0                           :  8 ;  /*     [15:8] */
      unsigned int   data                              :  8 ;  /*      [7:0] */
   } b_tx8 ;

#define DUART_B_TX8_DATA_MASK                                            0x00FF

   struct
   {
      unsigned int   data                              : 16 ;  /*     [15:0] */
   } b_tx16 ;

#define DUART_B_TX16_DATA_MASK                                           0xFFFF

} ;



struct __dusart_fd_tag
{
   struct
   {
      unsigned int   __fill0                           : 10 ;  /*     [15:6] */
      unsigned int   parity                            :  2 ;  /*      [5:4] */
      unsigned int   endian                            :  1 ;  /*        [3] */
      unsigned int   protocol                          :  3 ;  /*      [2:0] */
   } a_cfg ;

#define DUSART_A_CFG_PROTOCOL_USR                              7
#define DUSART_A_CFG_PROTOCOL_UART                             4
#define DUSART_A_CFG_PROTOCOL_SCI                              3
#define DUSART_A_CFG_PROTOCOL_IFR                              2
#define DUSART_A_CFG_PROTOCOL_SPI                              1
#define DUSART_A_CFG_PROTOCOL_I2C                              0

#define DUSART_A_CFG_ENDIAN_BIG                                1
#define DUSART_A_CFG_ENDIAN_LITTLE                             0

#define DUSART_A_CFG_PARITY_ODD                                3
#define DUSART_A_CFG_PARITY_EVEN                               1
#define DUSART_A_CFG_PARITY_NONE                               0

#define DUSART_A_CFG_PROTOCOL_MASK                                       0x0007
#define DUSART_A_CFG_ENDIAN_MASK                                         0x0008
#define DUSART_A_CFG_PARITY_MASK                                         0x0030

   struct
   {
      unsigned int   __fill0                           :  2 ;  /*    [15:14] */
      unsigned int   flt2                              :  2 ;  /*    [13:12] */
      unsigned int   flt1                              :  2 ;  /*    [11:10] */
      unsigned int   flt0                              :  2 ;  /*      [9:8] */
      unsigned int   period                            :  8 ;  /*      [7:0] */
   } a_smpl_cfg ;

#define DUSART_A_SMPL_CFG_FLT0_MAJORITY                        3
#define DUSART_A_SMPL_CFG_FLT0_THREE                           2
#define DUSART_A_SMPL_CFG_FLT0_TWO                             1
#define DUSART_A_SMPL_CFG_FLT0_NONE                            0

#define DUSART_A_SMPL_CFG_FLT1_MAJORITY                        3
#define DUSART_A_SMPL_CFG_FLT1_THREE                           2
#define DUSART_A_SMPL_CFG_FLT1_TWO                             1
#define DUSART_A_SMPL_CFG_FLT1_NONE                            0

#define DUSART_A_SMPL_CFG_FLT2_MAJORITY                        3
#define DUSART_A_SMPL_CFG_FLT2_THREE                           2
#define DUSART_A_SMPL_CFG_FLT2_TWO                             1
#define DUSART_A_SMPL_CFG_FLT2_NONE                            0

#define DUSART_A_SMPL_CFG_PERIOD_MASK                                    0x00FF
#define DUSART_A_SMPL_CFG_FLT0_MASK                                      0x0300
#define DUSART_A_SMPL_CFG_FLT1_MASK                                      0x0C00
#define DUSART_A_SMPL_CFG_FLT2_MASK                                      0x3000

   struct
   {
      unsigned int   clk_low                           :  8 ;  /*     [15:8] */
      unsigned int   clk_high                          :  8 ;  /*      [7:0] */
   } a_sym_cfg ;

#define DUSART_A_SYM_CFG_CLK_HIGH_MASK                                   0x00FF
#define DUSART_A_SYM_CFG_CLK_LOW_MASK                                    0xFF00

   struct
   {
      unsigned int   tmo                               :  8 ;  /*     [15:8] */
      unsigned int   guard                             :  8 ;  /*      [7:0] */
   } a_tim_cfg ;

#define DUSART_A_TIM_CFG_GUARD_MASK                                      0x00FF
#define DUSART_A_TIM_CFG_TMO_MASK                                        0xFF00

   struct
   {
      unsigned int   __fill0                           :  8 ;  /*     [15:8] */
      unsigned int   tx_data                           :  8 ;  /*      [7:0] */
   } a0_tx8 ;

#define DUSART_A0_TX8_TX_DATA_MASK                                       0x00FF

   struct
   {
      unsigned int   tx_data                           : 16 ;  /*     [15:0] */
   } a0_tx16 ;

#define DUSART_A0_TX16_TX_DATA_MASK                                      0xFFFF

   struct
   {
      unsigned int   __fill0                           :  8 ;  /*     [15:8] */
      unsigned int   tx_data                           :  8 ;  /*      [7:0] */
   } a0_tx8_last ;

#define DUSART_A0_TX8_LAST_TX_DATA_MASK                                  0x00FF

   struct
   {
      unsigned int   tx_data                           : 16 ;  /*     [15:0] */
   } a0_tx16_last ;

#define DUSART_A0_TX16_LAST_TX_DATA_MASK                                 0xFFFF

   struct
   {
      unsigned int   __fill0                           :  8 ;  /*     [15:8] */
      unsigned int   tx_data                           :  8 ;  /*      [7:0] */
   } a1_tx8 ;

#define DUSART_A1_TX8_TX_DATA_MASK                                       0x00FF

   struct
   {
      unsigned int   tx_data                           : 16 ;  /*     [15:0] */
   } a1_tx16 ;

#define DUSART_A1_TX16_TX_DATA_MASK                                      0xFFFF

   struct
   {
      unsigned int   __fill0                           :  8 ;  /*     [15:8] */
      unsigned int   tx_data                           :  8 ;  /*      [7:0] */
   } a1_tx8_last ;

#define DUSART_A1_TX8_LAST_TX_DATA_MASK                                  0x00FF

   struct
   {
      unsigned int   tx_data                           : 16 ;  /*     [15:0] */
   } a1_tx16_last ;

#define DUSART_A1_TX16_LAST_TX_DATA_MASK                                 0xFFFF

   struct
   {
      unsigned int   __fill0                           :  8 ;  /*     [15:8] */
      unsigned int   rx_data                           :  8 ;  /*      [7:0] */
   } a0_rx8 ;

#define DUSART_A0_RX8_RX_DATA_MASK                                       0x00FF

   struct
   {
      unsigned int   rx_data                           : 16 ;  /*     [15:0] */
   } a0_rx16 ;

#define DUSART_A0_RX16_RX_DATA_MASK                                      0xFFFF

   struct
   {
      unsigned int   __fill0                           :  8 ;  /*     [15:8] */
      unsigned int   rx_data                           :  8 ;  /*      [7:0] */
   } a0_rx8_last ;

#define DUSART_A0_RX8_LAST_RX_DATA_MASK                                  0x00FF

   struct
   {
      unsigned int   rx_data                           : 16 ;  /*     [15:0] */
   } a0_rx16_last ;

#define DUSART_A0_RX16_LAST_RX_DATA_MASK                                 0xFFFF

   struct
   {
      unsigned int   __fill0                           :  8 ;  /*     [15:8] */
      unsigned int   rx_data                           :  8 ;  /*      [7:0] */
   } a1_rx8 ;

#define DUSART_A1_RX8_RX_DATA_MASK                                       0x00FF

   struct
   {
      unsigned int   rx_data                           : 16 ;  /*     [15:0] */
   } a1_rx16 ;

#define DUSART_A1_RX16_RX_DATA_MASK                                      0xFFFF

   struct
   {
      unsigned int   __fill0                           :  8 ;  /*     [15:8] */
      unsigned int   rx_data                           :  8 ;  /*      [7:0] */
   } a1_rx8_last ;

#define DUSART_A1_RX8_LAST_RX_DATA_MASK                                  0x00FF

   struct
   {
      unsigned int   rx_data                           : 16 ;  /*     [15:0] */
   } a1_rx16_last ;

#define DUSART_A1_RX16_LAST_RX_DATA_MASK                                 0xFFFF

   struct
   {
      unsigned int   __fill0                           :  5 ;  /*    [15:11] */
      unsigned int   rx_edge_det                       :  1 ;  /*       [10] */
      unsigned int   tx_cnt_done                       :  1 ;  /*        [9] */
      unsigned int   rx_cnt_done                       :  1 ;  /*        [8] */
      unsigned int   __fill1                           :  2 ;  /*      [7:6] */
      unsigned int   rx1_2b_rdy                        :  1 ;  /*        [5] */
      unsigned int   rx1_1b_rdy                        :  1 ;  /*        [4] */
      unsigned int   rx0_2b_rdy                        :  1 ;  /*        [3] */
      unsigned int   rx0_1b_rdy                        :  1 ;  /*        [2] */
      unsigned int   tx1_rdy                           :  1 ;  /*        [1] */
      unsigned int   tx0_rdy                           :  1 ;  /*        [0] */
   } a_int_sts ;

#define DUSART_A_INT_STS_TX0_RDY_MASK                                    0x0001
#define DUSART_A_INT_STS_TX1_RDY_MASK                                    0x0002
#define DUSART_A_INT_STS_RX0_1B_RDY_MASK                                 0x0004
#define DUSART_A_INT_STS_RX0_2B_RDY_MASK                                 0x0008
#define DUSART_A_INT_STS_RX1_1B_RDY_MASK                                 0x0010
#define DUSART_A_INT_STS_RX1_2B_RDY_MASK                                 0x0020
#define DUSART_A_INT_STS_RX_CNT_DONE_MASK                                0x0100
#define DUSART_A_INT_STS_TX_CNT_DONE_MASK                                0x0200
#define DUSART_A_INT_STS_RX_EDGE_DET_MASK                                0x0400

   struct
   {
      unsigned int   __fill0                           :  5 ;  /*    [15:11] */
      unsigned int   rx_edge_det                       :  1 ;  /*       [10] */
      unsigned int   tx_cnt_done                       :  1 ;  /*        [9] */
      unsigned int   rx_cnt_done                       :  1 ;  /*        [8] */
      unsigned int   __fill1                           :  2 ;  /*      [7:6] */
      unsigned int   rx1_2b_rdy                        :  1 ;  /*        [5] */
      unsigned int   rx1_1b_rdy                        :  1 ;  /*        [4] */
      unsigned int   rx0_2b_rdy                        :  1 ;  /*        [3] */
      unsigned int   rx0_1b_rdy                        :  1 ;  /*        [2] */
      unsigned int   tx1_rdy                           :  1 ;  /*        [1] */
      unsigned int   tx0_rdy                           :  1 ;  /*        [0] */
   } a_int_en ;

#define DUSART_A_INT_EN_TX0_RDY_MASK                                     0x0001
#define DUSART_A_INT_EN_TX1_RDY_MASK                                     0x0002
#define DUSART_A_INT_EN_RX0_1B_RDY_MASK                                  0x0004
#define DUSART_A_INT_EN_RX0_2B_RDY_MASK                                  0x0008
#define DUSART_A_INT_EN_RX1_1B_RDY_MASK                                  0x0010
#define DUSART_A_INT_EN_RX1_2B_RDY_MASK                                  0x0020
#define DUSART_A_INT_EN_RX_CNT_DONE_MASK                                 0x0100
#define DUSART_A_INT_EN_TX_CNT_DONE_MASK                                 0x0200
#define DUSART_A_INT_EN_RX_EDGE_DET_MASK                                 0x0400

   struct
   {
      unsigned int   __fill0                           :  5 ;  /*    [15:11] */
      unsigned int   rx_edge_det                       :  1 ;  /*       [10] */
      unsigned int   tx_cnt_done                       :  1 ;  /*        [9] */
      unsigned int   rx_cnt_done                       :  1 ;  /*        [8] */
      unsigned int   __fill1                           :  2 ;  /*      [7:6] */
      unsigned int   rx1_2b_rdy                        :  1 ;  /*        [5] */
      unsigned int   rx1_1b_rdy                        :  1 ;  /*        [4] */
      unsigned int   rx0_2b_rdy                        :  1 ;  /*        [3] */
      unsigned int   rx0_1b_rdy                        :  1 ;  /*        [2] */
      unsigned int   tx1_rdy                           :  1 ;  /*        [1] */
      unsigned int   tx0_rdy                           :  1 ;  /*        [0] */
   } a_int_dis ;

#define DUSART_A_INT_DIS_TX0_RDY_MASK                                    0x0001
#define DUSART_A_INT_DIS_TX1_RDY_MASK                                    0x0002
#define DUSART_A_INT_DIS_RX0_1B_RDY_MASK                                 0x0004
#define DUSART_A_INT_DIS_RX0_2B_RDY_MASK                                 0x0008
#define DUSART_A_INT_DIS_RX1_1B_RDY_MASK                                 0x0010
#define DUSART_A_INT_DIS_RX1_2B_RDY_MASK                                 0x0020
#define DUSART_A_INT_DIS_RX_CNT_DONE_MASK                                0x0100
#define DUSART_A_INT_DIS_TX_CNT_DONE_MASK                                0x0200
#define DUSART_A_INT_DIS_RX_EDGE_DET_MASK                                0x0400

   struct
   {
      unsigned int   __fill0                           :  5 ;  /*    [15:11] */
      unsigned int   rx_edge_det                       :  1 ;  /*       [10] */
      unsigned int   tx_cnt_done                       :  1 ;  /*        [9] */
      unsigned int   rx_cnt_done                       :  1 ;  /*        [8] */
      unsigned int   __fill1                           :  8 ;  /*      [7:0] */
   } a_int_clr ;

#define DUSART_A_INT_CLR_RX_CNT_DONE_MASK                                0x0100
#define DUSART_A_INT_CLR_TX_CNT_DONE_MASK                                0x0200
#define DUSART_A_INT_CLR_RX_EDGE_DET_MASK                                0x0400

   struct
   {
      unsigned int   __fill0                           :  4 ;  /*    [15:12] */
      unsigned int   rx_brk                            :  1 ;  /*       [11] */
      unsigned int   frame_tmo                         :  1 ;  /*       [10] */
      unsigned int   frame_err                         :  1 ;  /*        [9] */
      unsigned int   rx_perr                           :  1 ;  /*        [8] */
      unsigned int   rx1_ufl                           :  1 ;  /*        [7] */
      unsigned int   rx0_ufl                           :  1 ;  /*        [6] */
      unsigned int   tx1_ufl                           :  1 ;  /*        [5] */
      unsigned int   tx0_ufl                           :  1 ;  /*        [4] */
      unsigned int   rx1_ofl                           :  1 ;  /*        [3] */
      unsigned int   rx0_ofl                           :  1 ;  /*        [2] */
      unsigned int   tx1_ofl                           :  1 ;  /*        [1] */
      unsigned int   tx0_ofl                           :  1 ;  /*        [0] */
   } a_ex_sts ;

#define DUSART_A_EX_STS_TX0_OFL_MASK                                     0x0001
#define DUSART_A_EX_STS_TX1_OFL_MASK                                     0x0002
#define DUSART_A_EX_STS_RX0_OFL_MASK                                     0x0004
#define DUSART_A_EX_STS_RX1_OFL_MASK                                     0x0008
#define DUSART_A_EX_STS_TX0_UFL_MASK                                     0x0010
#define DUSART_A_EX_STS_TX1_UFL_MASK                                     0x0020
#define DUSART_A_EX_STS_RX0_UFL_MASK                                     0x0040
#define DUSART_A_EX_STS_RX1_UFL_MASK                                     0x0080
#define DUSART_A_EX_STS_RX_PERR_MASK                                     0x0100
#define DUSART_A_EX_STS_FRAME_ERR_MASK                                   0x0200
#define DUSART_A_EX_STS_FRAME_TMO_MASK                                   0x0400
#define DUSART_A_EX_STS_RX_BRK_MASK                                      0x0800

   struct
   {
      unsigned int   __fill0                           :  4 ;  /*    [15:12] */
      unsigned int   rx_brk                            :  1 ;  /*       [11] */
      unsigned int   frame_tmo                         :  1 ;  /*       [10] */
      unsigned int   frame_err                         :  1 ;  /*        [9] */
      unsigned int   rx_perr                           :  1 ;  /*        [8] */
      unsigned int   rx1_ufl                           :  1 ;  /*        [7] */
      unsigned int   rx0_ufl                           :  1 ;  /*        [6] */
      unsigned int   tx1_ufl                           :  1 ;  /*        [5] */
      unsigned int   tx0_ufl                           :  1 ;  /*        [4] */
      unsigned int   rx1_ofl                           :  1 ;  /*        [3] */
      unsigned int   rx0_ofl                           :  1 ;  /*        [2] */
      unsigned int   tx1_ofl                           :  1 ;  /*        [1] */
      unsigned int   tx0_ofl                           :  1 ;  /*        [0] */
   } a_ex_en ;

#define DUSART_A_EX_EN_TX0_OFL_MASK                                      0x0001
#define DUSART_A_EX_EN_TX1_OFL_MASK                                      0x0002
#define DUSART_A_EX_EN_RX0_OFL_MASK                                      0x0004
#define DUSART_A_EX_EN_RX1_OFL_MASK                                      0x0008
#define DUSART_A_EX_EN_TX0_UFL_MASK                                      0x0010
#define DUSART_A_EX_EN_TX1_UFL_MASK                                      0x0020
#define DUSART_A_EX_EN_RX0_UFL_MASK                                      0x0040
#define DUSART_A_EX_EN_RX1_UFL_MASK                                      0x0080
#define DUSART_A_EX_EN_RX_PERR_MASK                                      0x0100
#define DUSART_A_EX_EN_FRAME_ERR_MASK                                    0x0200
#define DUSART_A_EX_EN_FRAME_TMO_MASK                                    0x0400
#define DUSART_A_EX_EN_RX_BRK_MASK                                       0x0800

   struct
   {
      unsigned int   __fill0                           :  4 ;  /*    [15:12] */
      unsigned int   rx_brk                            :  1 ;  /*       [11] */
      unsigned int   frame_tmo                         :  1 ;  /*       [10] */
      unsigned int   frame_err                         :  1 ;  /*        [9] */
      unsigned int   rx_perr                           :  1 ;  /*        [8] */
      unsigned int   rx1_ufl                           :  1 ;  /*        [7] */
      unsigned int   rx0_ufl                           :  1 ;  /*        [6] */
      unsigned int   tx1_ufl                           :  1 ;  /*        [5] */
      unsigned int   tx0_ufl                           :  1 ;  /*        [4] */
      unsigned int   rx1_ofl                           :  1 ;  /*        [3] */
      unsigned int   rx0_ofl                           :  1 ;  /*        [2] */
      unsigned int   tx1_ofl                           :  1 ;  /*        [1] */
      unsigned int   tx0_ofl                           :  1 ;  /*        [0] */
   } a_ex_dis ;

#define DUSART_A_EX_DIS_TX0_OFL_MASK                                     0x0001
#define DUSART_A_EX_DIS_TX1_OFL_MASK                                     0x0002
#define DUSART_A_EX_DIS_RX0_OFL_MASK                                     0x0004
#define DUSART_A_EX_DIS_RX1_OFL_MASK                                     0x0008
#define DUSART_A_EX_DIS_TX0_UFL_MASK                                     0x0010
#define DUSART_A_EX_DIS_TX1_UFL_MASK                                     0x0020
#define DUSART_A_EX_DIS_RX0_UFL_MASK                                     0x0040
#define DUSART_A_EX_DIS_RX1_UFL_MASK                                     0x0080
#define DUSART_A_EX_DIS_RX_PERR_MASK                                     0x0100
#define DUSART_A_EX_DIS_FRAME_ERR_MASK                                   0x0200
#define DUSART_A_EX_DIS_FRAME_TMO_MASK                                   0x0400
#define DUSART_A_EX_DIS_RX_BRK_MASK                                      0x0800

   struct
   {
      unsigned int   __fill0                           :  4 ;  /*    [15:12] */
      unsigned int   rx_brk                            :  1 ;  /*       [11] */
      unsigned int   frame_tmo                         :  1 ;  /*       [10] */
      unsigned int   frame_err                         :  1 ;  /*        [9] */
      unsigned int   rx_perr                           :  1 ;  /*        [8] */
      unsigned int   rx1_ufl                           :  1 ;  /*        [7] */
      unsigned int   rx0_ufl                           :  1 ;  /*        [6] */
      unsigned int   tx1_ufl                           :  1 ;  /*        [5] */
      unsigned int   tx0_ufl                           :  1 ;  /*        [4] */
      unsigned int   rx1_ofl                           :  1 ;  /*        [3] */
      unsigned int   rx0_ofl                           :  1 ;  /*        [2] */
      unsigned int   tx1_ofl                           :  1 ;  /*        [1] */
      unsigned int   tx0_ofl                           :  1 ;  /*        [0] */
   } a_ex_clr ;

#define DUSART_A_EX_CLR_TX0_OFL_MASK                                     0x0001
#define DUSART_A_EX_CLR_TX1_OFL_MASK                                     0x0002
#define DUSART_A_EX_CLR_RX0_OFL_MASK                                     0x0004
#define DUSART_A_EX_CLR_RX1_OFL_MASK                                     0x0008
#define DUSART_A_EX_CLR_TX0_UFL_MASK                                     0x0010
#define DUSART_A_EX_CLR_TX1_UFL_MASK                                     0x0020
#define DUSART_A_EX_CLR_RX0_UFL_MASK                                     0x0040
#define DUSART_A_EX_CLR_RX1_UFL_MASK                                     0x0080
#define DUSART_A_EX_CLR_RX_PERR_MASK                                     0x0100
#define DUSART_A_EX_CLR_FRAME_ERR_MASK                                   0x0200
#define DUSART_A_EX_CLR_FRAME_TMO_MASK                                   0x0400
#define DUSART_A_EX_CLR_RX_BRK_MASK                                      0x0800

   struct
   {
      unsigned int   __fill0                           : 10 ;  /*     [15:6] */
      unsigned int   parity                            :  2 ;  /*      [5:4] */
      unsigned int   endian                            :  1 ;  /*        [3] */
      unsigned int   protocol                          :  3 ;  /*      [2:0] */
   } b_cfg ;

#define DUSART_B_CFG_PROTOCOL_USR                              7
#define DUSART_B_CFG_PROTOCOL_UART                             4
#define DUSART_B_CFG_PROTOCOL_SCI                              3
#define DUSART_B_CFG_PROTOCOL_IFR                              2
#define DUSART_B_CFG_PROTOCOL_SPI                              1
#define DUSART_B_CFG_PROTOCOL_I2C                              0

#define DUSART_B_CFG_ENDIAN_BIG                                1
#define DUSART_B_CFG_ENDIAN_LITTLE                             0

#define DUSART_B_CFG_PARITY_ODD                                3
#define DUSART_B_CFG_PARITY_EVEN                               1
#define DUSART_B_CFG_PARITY_NONE                               0

#define DUSART_B_CFG_PROTOCOL_MASK                                       0x0007
#define DUSART_B_CFG_ENDIAN_MASK                                         0x0008
#define DUSART_B_CFG_PARITY_MASK                                         0x0030

   struct
   {
      unsigned int   __fill0                           :  2 ;  /*    [15:14] */
      unsigned int   flt2                              :  2 ;  /*    [13:12] */
      unsigned int   flt1                              :  2 ;  /*    [11:10] */
      unsigned int   flt0                              :  2 ;  /*      [9:8] */
      unsigned int   period                            :  8 ;  /*      [7:0] */
   } b_smpl_cfg ;

#define DUSART_B_SMPL_CFG_FLT0_MAJORITY                        3
#define DUSART_B_SMPL_CFG_FLT0_THREE                           2
#define DUSART_B_SMPL_CFG_FLT0_TWO                             1
#define DUSART_B_SMPL_CFG_FLT0_NONE                            0

#define DUSART_B_SMPL_CFG_FLT1_MAJORITY                        3
#define DUSART_B_SMPL_CFG_FLT1_THREE                           2
#define DUSART_B_SMPL_CFG_FLT1_TWO                             1
#define DUSART_B_SMPL_CFG_FLT1_NONE                            0

#define DUSART_B_SMPL_CFG_FLT2_MAJORITY                        3
#define DUSART_B_SMPL_CFG_FLT2_THREE                           2
#define DUSART_B_SMPL_CFG_FLT2_TWO                             1
#define DUSART_B_SMPL_CFG_FLT2_NONE                            0

#define DUSART_B_SMPL_CFG_PERIOD_MASK                                    0x00FF
#define DUSART_B_SMPL_CFG_FLT0_MASK                                      0x0300
#define DUSART_B_SMPL_CFG_FLT1_MASK                                      0x0C00
#define DUSART_B_SMPL_CFG_FLT2_MASK                                      0x3000

   struct
   {
      unsigned int   clk_low                           :  8 ;  /*     [15:8] */
      unsigned int   clk_high                          :  8 ;  /*      [7:0] */
   } b_sym_cfg ;

#define DUSART_B_SYM_CFG_CLK_HIGH_MASK                                   0x00FF
#define DUSART_B_SYM_CFG_CLK_LOW_MASK                                    0xFF00

   struct
   {
      unsigned int   tmo                               :  8 ;  /*     [15:8] */
      unsigned int   guard                             :  8 ;  /*      [7:0] */
   } b_tim_cfg ;

#define DUSART_B_TIM_CFG_GUARD_MASK                                      0x00FF
#define DUSART_B_TIM_CFG_TMO_MASK                                        0xFF00

   struct
   {
      unsigned int   __fill0                           :  8 ;  /*     [15:8] */
      unsigned int   tx_data                           :  8 ;  /*      [7:0] */
   } b0_tx8 ;

#define DUSART_B0_TX8_TX_DATA_MASK                                       0x00FF

   struct
   {
      unsigned int   tx_data                           : 16 ;  /*     [15:0] */
   } b0_tx16 ;

#define DUSART_B0_TX16_TX_DATA_MASK                                      0xFFFF

   struct
   {
      unsigned int   __fill0                           :  8 ;  /*     [15:8] */
      unsigned int   tx_data                           :  8 ;  /*      [7:0] */
   } b0_tx8_last ;

#define DUSART_B0_TX8_LAST_TX_DATA_MASK                                  0x00FF

   struct
   {
      unsigned int   tx_data                           : 16 ;  /*     [15:0] */
   } b0_tx16_last ;

#define DUSART_B0_TX16_LAST_TX_DATA_MASK                                 0xFFFF

   struct
   {
      unsigned int   __fill0                           :  8 ;  /*     [15:8] */
      unsigned int   tx_data                           :  8 ;  /*      [7:0] */
   } b1_tx8 ;

#define DUSART_B1_TX8_TX_DATA_MASK                                       0x00FF

   struct
   {
      unsigned int   tx_data                           : 16 ;  /*     [15:0] */
   } b1_tx16 ;

#define DUSART_B1_TX16_TX_DATA_MASK                                      0xFFFF

   struct
   {
      unsigned int   __fill0                           :  8 ;  /*     [15:8] */
      unsigned int   tx_data                           :  8 ;  /*      [7:0] */
   } b1_tx8_last ;

#define DUSART_B1_TX8_LAST_TX_DATA_MASK                                  0x00FF

   struct
   {
      unsigned int   tx_data                           : 16 ;  /*     [15:0] */
   } b1_tx16_last ;

#define DUSART_B1_TX16_LAST_TX_DATA_MASK                                 0xFFFF

   struct
   {
      unsigned int   __fill0                           :  8 ;  /*     [15:8] */
      unsigned int   rx_data                           :  8 ;  /*      [7:0] */
   } b0_rx8 ;

#define DUSART_B0_RX8_RX_DATA_MASK                                       0x00FF

   struct
   {
      unsigned int   rx_data                           : 16 ;  /*     [15:0] */
   } b0_rx16 ;

#define DUSART_B0_RX16_RX_DATA_MASK                                      0xFFFF

   struct
   {
      unsigned int   __fill0                           :  8 ;  /*     [15:8] */
      unsigned int   rx_data                           :  8 ;  /*      [7:0] */
   } b0_rx8_last ;

#define DUSART_B0_RX8_LAST_RX_DATA_MASK                                  0x00FF

   struct
   {
      unsigned int   rx_data                           : 16 ;  /*     [15:0] */
   } b0_rx16_last ;

#define DUSART_B0_RX16_LAST_RX_DATA_MASK                                 0xFFFF

   struct
   {
      unsigned int   __fill0                           :  8 ;  /*     [15:8] */
      unsigned int   rx_data                           :  8 ;  /*      [7:0] */
   } b1_rx8 ;

#define DUSART_B1_RX8_RX_DATA_MASK                                       0x00FF

   struct
   {
      unsigned int   rx_data                           : 16 ;  /*     [15:0] */
   } b1_rx16 ;

#define DUSART_B1_RX16_RX_DATA_MASK                                      0xFFFF

   struct
   {
      unsigned int   __fill0                           :  8 ;  /*     [15:8] */
      unsigned int   rx_data                           :  8 ;  /*      [7:0] */
   } b1_rx8_last ;

#define DUSART_B1_RX8_LAST_RX_DATA_MASK                                  0x00FF

   struct
   {
      unsigned int   rx_data                           : 16 ;  /*     [15:0] */
   } b1_rx16_last ;

#define DUSART_B1_RX16_LAST_RX_DATA_MASK                                 0xFFFF

   struct
   {
      unsigned int   __fill0                           :  5 ;  /*    [15:11] */
      unsigned int   rx_edge_det                       :  1 ;  /*       [10] */
      unsigned int   tx_cnt_done                       :  1 ;  /*        [9] */
      unsigned int   rx_cnt_done                       :  1 ;  /*        [8] */
      unsigned int   __fill1                           :  2 ;  /*      [7:6] */
      unsigned int   rx1_2b_rdy                        :  1 ;  /*        [5] */
      unsigned int   rx1_1b_rdy                        :  1 ;  /*        [4] */
      unsigned int   rx0_2b_rdy                        :  1 ;  /*        [3] */
      unsigned int   rx0_1b_rdy                        :  1 ;  /*        [2] */
      unsigned int   tx1_rdy                           :  1 ;  /*        [1] */
      unsigned int   tx0_rdy                           :  1 ;  /*        [0] */
   } b_int_sts ;

#define DUSART_B_INT_STS_TX0_RDY_MASK                                    0x0001
#define DUSART_B_INT_STS_TX1_RDY_MASK                                    0x0002
#define DUSART_B_INT_STS_RX0_1B_RDY_MASK                                 0x0004
#define DUSART_B_INT_STS_RX0_2B_RDY_MASK                                 0x0008
#define DUSART_B_INT_STS_RX1_1B_RDY_MASK                                 0x0010
#define DUSART_B_INT_STS_RX1_2B_RDY_MASK                                 0x0020
#define DUSART_B_INT_STS_RX_CNT_DONE_MASK                                0x0100
#define DUSART_B_INT_STS_TX_CNT_DONE_MASK                                0x0200
#define DUSART_B_INT_STS_RX_EDGE_DET_MASK                                0x0400

   struct
   {
      unsigned int   __fill0                           :  5 ;  /*    [15:11] */
      unsigned int   rx_edge_det                       :  1 ;  /*       [10] */
      unsigned int   tx_cnt_done                       :  1 ;  /*        [9] */
      unsigned int   rx_cnt_done                       :  1 ;  /*        [8] */
      unsigned int   __fill1                           :  2 ;  /*      [7:6] */
      unsigned int   rx1_2b_rdy                        :  1 ;  /*        [5] */
      unsigned int   rx1_1b_rdy                        :  1 ;  /*        [4] */
      unsigned int   rx0_2b_rdy                        :  1 ;  /*        [3] */
      unsigned int   rx0_1b_rdy                        :  1 ;  /*        [2] */
      unsigned int   tx1_rdy                           :  1 ;  /*        [1] */
      unsigned int   tx0_rdy                           :  1 ;  /*        [0] */
   } b_int_en ;

#define DUSART_B_INT_EN_TX0_RDY_MASK                                     0x0001
#define DUSART_B_INT_EN_TX1_RDY_MASK                                     0x0002
#define DUSART_B_INT_EN_RX0_1B_RDY_MASK                                  0x0004
#define DUSART_B_INT_EN_RX0_2B_RDY_MASK                                  0x0008
#define DUSART_B_INT_EN_RX1_1B_RDY_MASK                                  0x0010
#define DUSART_B_INT_EN_RX1_2B_RDY_MASK                                  0x0020
#define DUSART_B_INT_EN_RX_CNT_DONE_MASK                                 0x0100
#define DUSART_B_INT_EN_TX_CNT_DONE_MASK                                 0x0200
#define DUSART_B_INT_EN_RX_EDGE_DET_MASK                                 0x0400

   struct
   {
      unsigned int   __fill0                           :  5 ;  /*    [15:11] */
      unsigned int   rx_edge_det                       :  1 ;  /*       [10] */
      unsigned int   tx_cnt_done                       :  1 ;  /*        [9] */
      unsigned int   rx_cnt_done                       :  1 ;  /*        [8] */
      unsigned int   __fill1                           :  2 ;  /*      [7:6] */
      unsigned int   rx1_2b_rdy                        :  1 ;  /*        [5] */
      unsigned int   rx1_1b_rdy                        :  1 ;  /*        [4] */
      unsigned int   rx0_2b_rdy                        :  1 ;  /*        [3] */
      unsigned int   rx0_1b_rdy                        :  1 ;  /*        [2] */
      unsigned int   tx1_rdy                           :  1 ;  /*        [1] */
      unsigned int   tx0_rdy                           :  1 ;  /*        [0] */
   } b_int_dis ;

#define DUSART_B_INT_DIS_TX0_RDY_MASK                                    0x0001
#define DUSART_B_INT_DIS_TX1_RDY_MASK                                    0x0002
#define DUSART_B_INT_DIS_RX0_1B_RDY_MASK                                 0x0004
#define DUSART_B_INT_DIS_RX0_2B_RDY_MASK                                 0x0008
#define DUSART_B_INT_DIS_RX1_1B_RDY_MASK                                 0x0010
#define DUSART_B_INT_DIS_RX1_2B_RDY_MASK                                 0x0020
#define DUSART_B_INT_DIS_RX_CNT_DONE_MASK                                0x0100
#define DUSART_B_INT_DIS_TX_CNT_DONE_MASK                                0x0200
#define DUSART_B_INT_DIS_RX_EDGE_DET_MASK                                0x0400

   struct
   {
      unsigned int   __fill0                           :  5 ;  /*    [15:11] */
      unsigned int   rx_edge_det                       :  1 ;  /*       [10] */
      unsigned int   tx_cnt_done                       :  1 ;  /*        [9] */
      unsigned int   rx_cnt_done                       :  1 ;  /*        [8] */
      unsigned int   __fill1                           :  8 ;  /*      [7:0] */
   } b_int_clr ;

#define DUSART_B_INT_CLR_RX_CNT_DONE_MASK                                0x0100
#define DUSART_B_INT_CLR_TX_CNT_DONE_MASK                                0x0200
#define DUSART_B_INT_CLR_RX_EDGE_DET_MASK                                0x0400

   struct
   {
      unsigned int   __fill0                           :  4 ;  /*    [15:12] */
      unsigned int   rx_brk                            :  1 ;  /*       [11] */
      unsigned int   frame_tmo                         :  1 ;  /*       [10] */
      unsigned int   frame_err                         :  1 ;  /*        [9] */
      unsigned int   rx_perr                           :  1 ;  /*        [8] */
      unsigned int   rx1_ufl                           :  1 ;  /*        [7] */
      unsigned int   rx0_ufl                           :  1 ;  /*        [6] */
      unsigned int   tx1_ufl                           :  1 ;  /*        [5] */
      unsigned int   tx0_ufl                           :  1 ;  /*        [4] */
      unsigned int   rx1_ofl                           :  1 ;  /*        [3] */
      unsigned int   rx0_ofl                           :  1 ;  /*        [2] */
      unsigned int   tx1_ofl                           :  1 ;  /*        [1] */
      unsigned int   tx0_ofl                           :  1 ;  /*        [0] */
   } b_ex_sts ;

#define DUSART_B_EX_STS_TX0_OFL_MASK                                     0x0001
#define DUSART_B_EX_STS_TX1_OFL_MASK                                     0x0002
#define DUSART_B_EX_STS_RX0_OFL_MASK                                     0x0004
#define DUSART_B_EX_STS_RX1_OFL_MASK                                     0x0008
#define DUSART_B_EX_STS_TX0_UFL_MASK                                     0x0010
#define DUSART_B_EX_STS_TX1_UFL_MASK                                     0x0020
#define DUSART_B_EX_STS_RX0_UFL_MASK                                     0x0040
#define DUSART_B_EX_STS_RX1_UFL_MASK                                     0x0080
#define DUSART_B_EX_STS_RX_PERR_MASK                                     0x0100
#define DUSART_B_EX_STS_FRAME_ERR_MASK                                   0x0200
#define DUSART_B_EX_STS_FRAME_TMO_MASK                                   0x0400
#define DUSART_B_EX_STS_RX_BRK_MASK                                      0x0800

   struct
   {
      unsigned int   __fill0                           :  4 ;  /*    [15:12] */
      unsigned int   rx_brk                            :  1 ;  /*       [11] */
      unsigned int   frame_tmo                         :  1 ;  /*       [10] */
      unsigned int   frame_err                         :  1 ;  /*        [9] */
      unsigned int   rx_perr                           :  1 ;  /*        [8] */
      unsigned int   rx1_ufl                           :  1 ;  /*        [7] */
      unsigned int   rx0_ufl                           :  1 ;  /*        [6] */
      unsigned int   tx1_ufl                           :  1 ;  /*        [5] */
      unsigned int   tx0_ufl                           :  1 ;  /*        [4] */
      unsigned int   rx1_ofl                           :  1 ;  /*        [3] */
      unsigned int   rx0_ofl                           :  1 ;  /*        [2] */
      unsigned int   tx1_ofl                           :  1 ;  /*        [1] */
      unsigned int   tx0_ofl                           :  1 ;  /*        [0] */
   } b_ex_en ;

#define DUSART_B_EX_EN_TX0_OFL_MASK                                      0x0001
#define DUSART_B_EX_EN_TX1_OFL_MASK                                      0x0002
#define DUSART_B_EX_EN_RX0_OFL_MASK                                      0x0004
#define DUSART_B_EX_EN_RX1_OFL_MASK                                      0x0008
#define DUSART_B_EX_EN_TX0_UFL_MASK                                      0x0010
#define DUSART_B_EX_EN_TX1_UFL_MASK                                      0x0020
#define DUSART_B_EX_EN_RX0_UFL_MASK                                      0x0040
#define DUSART_B_EX_EN_RX1_UFL_MASK                                      0x0080
#define DUSART_B_EX_EN_RX_PERR_MASK                                      0x0100
#define DUSART_B_EX_EN_FRAME_ERR_MASK                                    0x0200
#define DUSART_B_EX_EN_FRAME_TMO_MASK                                    0x0400
#define DUSART_B_EX_EN_RX_BRK_MASK                                       0x0800

   struct
   {
      unsigned int   __fill0                           :  4 ;  /*    [15:12] */
      unsigned int   rx_brk                            :  1 ;  /*       [11] */
      unsigned int   frame_tmo                         :  1 ;  /*       [10] */
      unsigned int   frame_err                         :  1 ;  /*        [9] */
      unsigned int   rx_perr                           :  1 ;  /*        [8] */
      unsigned int   rx1_ufl                           :  1 ;  /*        [7] */
      unsigned int   rx0_ufl                           :  1 ;  /*        [6] */
      unsigned int   tx1_ufl                           :  1 ;  /*        [5] */
      unsigned int   tx0_ufl                           :  1 ;  /*        [4] */
      unsigned int   rx1_ofl                           :  1 ;  /*        [3] */
      unsigned int   rx0_ofl                           :  1 ;  /*        [2] */
      unsigned int   tx1_ofl                           :  1 ;  /*        [1] */
      unsigned int   tx0_ofl                           :  1 ;  /*        [0] */
   } b_ex_dis ;

#define DUSART_B_EX_DIS_TX0_OFL_MASK                                     0x0001
#define DUSART_B_EX_DIS_TX1_OFL_MASK                                     0x0002
#define DUSART_B_EX_DIS_RX0_OFL_MASK                                     0x0004
#define DUSART_B_EX_DIS_RX1_OFL_MASK                                     0x0008
#define DUSART_B_EX_DIS_TX0_UFL_MASK                                     0x0010
#define DUSART_B_EX_DIS_TX1_UFL_MASK                                     0x0020
#define DUSART_B_EX_DIS_RX0_UFL_MASK                                     0x0040
#define DUSART_B_EX_DIS_RX1_UFL_MASK                                     0x0080
#define DUSART_B_EX_DIS_RX_PERR_MASK                                     0x0100
#define DUSART_B_EX_DIS_FRAME_ERR_MASK                                   0x0200
#define DUSART_B_EX_DIS_FRAME_TMO_MASK                                   0x0400
#define DUSART_B_EX_DIS_RX_BRK_MASK                                      0x0800

   struct
   {
      unsigned int   __fill0                           :  4 ;  /*    [15:12] */
      unsigned int   rx_brk                            :  1 ;  /*       [11] */
      unsigned int   frame_tmo                         :  1 ;  /*       [10] */
      unsigned int   frame_err                         :  1 ;  /*        [9] */
      unsigned int   rx_perr                           :  1 ;  /*        [8] */
      unsigned int   rx1_ufl                           :  1 ;  /*        [7] */
      unsigned int   rx0_ufl                           :  1 ;  /*        [6] */
      unsigned int   tx1_ufl                           :  1 ;  /*        [5] */
      unsigned int   tx0_ufl                           :  1 ;  /*        [4] */
      unsigned int   rx1_ofl                           :  1 ;  /*        [3] */
      unsigned int   rx0_ofl                           :  1 ;  /*        [2] */
      unsigned int   tx1_ofl                           :  1 ;  /*        [1] */
      unsigned int   tx0_ofl                           :  1 ;  /*        [0] */
   } b_ex_clr ;

#define DUSART_B_EX_CLR_TX0_OFL_MASK                                     0x0001
#define DUSART_B_EX_CLR_TX1_OFL_MASK                                     0x0002
#define DUSART_B_EX_CLR_RX0_OFL_MASK                                     0x0004
#define DUSART_B_EX_CLR_RX1_OFL_MASK                                     0x0008
#define DUSART_B_EX_CLR_TX0_UFL_MASK                                     0x0010
#define DUSART_B_EX_CLR_TX1_UFL_MASK                                     0x0020
#define DUSART_B_EX_CLR_RX0_UFL_MASK                                     0x0040
#define DUSART_B_EX_CLR_RX1_UFL_MASK                                     0x0080
#define DUSART_B_EX_CLR_RX_PERR_MASK                                     0x0100
#define DUSART_B_EX_CLR_FRAME_ERR_MASK                                   0x0200
#define DUSART_B_EX_CLR_FRAME_TMO_MASK                                   0x0400
#define DUSART_B_EX_CLR_RX_BRK_MASK                                      0x0800

   struct
   {
      unsigned int   __fill0                           : 13 ;  /*     [15:3] */
      unsigned int   hold_en                           :  1 ;  /*        [2] */
      unsigned int   samp_del                          :  2 ;  /*      [1:0] */
   } i2c_cfg ;

#define DUSART_I2C_CFG_SAMP_DEL_MASK                                     0x0003
#define DUSART_I2C_CFG_HOLD_EN_MASK                                      0x0004

   struct
   {
      unsigned int   slave_en                          :  1 ;  /*       [15] */
      unsigned int   __fill0                           :  2 ;  /*    [14:13] */
      unsigned int   gen_call_en                       :  1 ;  /*       [12] */
      unsigned int   __fill1                           :  1 ;  /*       [11] */
      unsigned int   adr_size                          :  1 ;  /*       [10] */
      unsigned int   adr                               : 10 ;  /*      [9:0] */
   } i2c_slave_cfg ;

#define DUSART_I2C_SLAVE_CFG_ADR_SIZE_TEN_BIT                  1
#define DUSART_I2C_SLAVE_CFG_ADR_SIZE_SEVEN_BIT                0

#define DUSART_I2C_SLAVE_CFG_ADR_MASK                                    0x03FF
#define DUSART_I2C_SLAVE_CFG_ADR_SIZE_MASK                               0x0400
#define DUSART_I2C_SLAVE_CFG_GEN_CALL_EN_MASK                            0x1000
#define DUSART_I2C_SLAVE_CFG_SLAVE_EN_MASK                               0x8000

   struct
   {
      unsigned int   __fill0                           :  1 ;  /*       [15] */
      unsigned int   abort                             :  1 ;  /*       [14] */
      unsigned int   start_byte_en                     :  1 ;  /*       [13] */
      unsigned int   gen_call_en                       :  1 ;  /*       [12] */
      unsigned int   dir                               :  1 ;  /*       [11] */
      unsigned int   adr_size                          :  1 ;  /*       [10] */
      unsigned int   adr                               : 10 ;  /*      [9:0] */
   } i2c_master_cmd ;

#define DUSART_I2C_MASTER_CMD_ADR_SIZE_TEN_BIT                 1
#define DUSART_I2C_MASTER_CMD_ADR_SIZE_SEVEN_BIT               0

#define DUSART_I2C_MASTER_CMD_DIR_READ                         1
#define DUSART_I2C_MASTER_CMD_DIR_WRITE                        0

#define DUSART_I2C_MASTER_CMD_ADR_MASK                                   0x03FF
#define DUSART_I2C_MASTER_CMD_ADR_SIZE_MASK                              0x0400
#define DUSART_I2C_MASTER_CMD_DIR_MASK                                   0x0800
#define DUSART_I2C_MASTER_CMD_GEN_CALL_EN_MASK                           0x1000
#define DUSART_I2C_MASTER_CMD_START_BYTE_EN_MASK                         0x2000
#define DUSART_I2C_MASTER_CMD_ABORT_MASK                                 0x4000

   struct
   {
      unsigned int   __fill0                           :  7 ;  /*     [15:9] */
      unsigned int   frame_pol                         :  1 ;  /*        [8] */
      unsigned int   frame_dur                         :  1 ;  /*        [7] */
      unsigned int   frame_mode                        :  1 ;  /*        [6] */
      unsigned int   frame_src                         :  1 ;  /*        [5] */
      unsigned int   clk_pha                           :  1 ;  /*        [4] */
      unsigned int   clk_pol                           :  1 ;  /*        [3] */
      unsigned int   clk_mode                          :  1 ;  /*        [2] */
      unsigned int   clk_src                           :  1 ;  /*        [1] */
      unsigned int   word_dur                          :  1 ;  /*        [0] */
   } spi_tx_cfg ;

#define DUSART_SPI_TX_CFG_WORD_DUR_VAR                         1
#define DUSART_SPI_TX_CFG_WORD_DUR_FIX                         0

#define DUSART_SPI_TX_CFG_CLK_SRC_EXT                          1
#define DUSART_SPI_TX_CFG_CLK_SRC_INT                          0

#define DUSART_SPI_TX_CFG_CLK_MODE_CONT                        1
#define DUSART_SPI_TX_CFG_CLK_MODE_BURST                       0

#define DUSART_SPI_TX_CFG_CLK_POL_ACT_HIGH                     1
#define DUSART_SPI_TX_CFG_CLK_POL_ACT_LOW                      0

#define DUSART_SPI_TX_CFG_CLK_PHA_ONE                          1
#define DUSART_SPI_TX_CFG_CLK_PHA_ZERO                         0

#define DUSART_SPI_TX_CFG_FRAME_SRC_EXT                        1
#define DUSART_SPI_TX_CFG_FRAME_SRC_INT                        0

#define DUSART_SPI_TX_CFG_FRAME_MODE_CONT                      1
#define DUSART_SPI_TX_CFG_FRAME_MODE_BURST                     0

#define DUSART_SPI_TX_CFG_FRAME_DUR_WORD_LONG                  1
#define DUSART_SPI_TX_CFG_FRAME_DUR_BIT_LONG                   0

#define DUSART_SPI_TX_CFG_FRAME_POL_ACT_HIGH                   1
#define DUSART_SPI_TX_CFG_FRAME_POL_ACT_LOW                    0

#define DUSART_SPI_TX_CFG_WORD_DUR_MASK                                  0x0001
#define DUSART_SPI_TX_CFG_CLK_SRC_MASK                                   0x0002
#define DUSART_SPI_TX_CFG_CLK_MODE_MASK                                  0x0004
#define DUSART_SPI_TX_CFG_CLK_POL_MASK                                   0x0008
#define DUSART_SPI_TX_CFG_CLK_PHA_MASK                                   0x0010
#define DUSART_SPI_TX_CFG_FRAME_SRC_MASK                                 0x0020
#define DUSART_SPI_TX_CFG_FRAME_MODE_MASK                                0x0040
#define DUSART_SPI_TX_CFG_FRAME_DUR_MASK                                 0x0080
#define DUSART_SPI_TX_CFG_FRAME_POL_MASK                                 0x0100

   struct
   {
      unsigned int   __fill0                           :  7 ;  /*     [15:9] */
      unsigned int   frame_pol                         :  1 ;  /*        [8] */
      unsigned int   frame_dur                         :  1 ;  /*        [7] */
      unsigned int   frame_mode                        :  1 ;  /*        [6] */
      unsigned int   frame_src                         :  1 ;  /*        [5] */
      unsigned int   clk_pha                           :  1 ;  /*        [4] */
      unsigned int   clk_pol                           :  1 ;  /*        [3] */
      unsigned int   clk_mode                          :  1 ;  /*        [2] */
      unsigned int   clk_src                           :  1 ;  /*        [1] */
      unsigned int   word_dur                          :  1 ;  /*        [0] */
   } spi_rx_cfg ;

#define DUSART_SPI_RX_CFG_WORD_DUR_VAR                         1
#define DUSART_SPI_RX_CFG_WORD_DUR_FIX                         0

#define DUSART_SPI_RX_CFG_CLK_SRC_EXT                          1
#define DUSART_SPI_RX_CFG_CLK_SRC_INT                          0

#define DUSART_SPI_RX_CFG_CLK_MODE_CONT                        1
#define DUSART_SPI_RX_CFG_CLK_MODE_BURST                       0

#define DUSART_SPI_RX_CFG_CLK_POL_ACT_HIGH                     1
#define DUSART_SPI_RX_CFG_CLK_POL_ACT_LOW                      0

#define DUSART_SPI_RX_CFG_CLK_PHA_ONE                          1
#define DUSART_SPI_RX_CFG_CLK_PHA_ZERO                         0

#define DUSART_SPI_RX_CFG_FRAME_SRC_EXT                        1
#define DUSART_SPI_RX_CFG_FRAME_SRC_INT                        0

#define DUSART_SPI_RX_CFG_FRAME_MODE_CONT                      1
#define DUSART_SPI_RX_CFG_FRAME_MODE_BURST                     0

#define DUSART_SPI_RX_CFG_FRAME_DUR_WORD_LONG                  1
#define DUSART_SPI_RX_CFG_FRAME_DUR_BIT_LONG                   0

#define DUSART_SPI_RX_CFG_FRAME_POL_ACT_HIGH                   1
#define DUSART_SPI_RX_CFG_FRAME_POL_ACT_LOW                    0

#define DUSART_SPI_RX_CFG_WORD_DUR_MASK                                  0x0001
#define DUSART_SPI_RX_CFG_CLK_SRC_MASK                                   0x0002
#define DUSART_SPI_RX_CFG_CLK_MODE_MASK                                  0x0004
#define DUSART_SPI_RX_CFG_CLK_POL_MASK                                   0x0008
#define DUSART_SPI_RX_CFG_CLK_PHA_MASK                                   0x0010
#define DUSART_SPI_RX_CFG_FRAME_SRC_MASK                                 0x0020
#define DUSART_SPI_RX_CFG_FRAME_MODE_MASK                                0x0040
#define DUSART_SPI_RX_CFG_FRAME_DUR_MASK                                 0x0080
#define DUSART_SPI_RX_CFG_FRAME_POL_MASK                                 0x0100

   struct
   {
      unsigned int   __fill0                           :  6 ;  /*    [15:10] */
      unsigned int   rx_dis                            :  1 ;  /*        [9] */
      unsigned int   rx_en                             :  1 ;  /*        [8] */
      unsigned int   __fill1                           :  6 ;  /*      [7:2] */
      unsigned int   tx_dis                            :  1 ;  /*        [1] */
      unsigned int   tx_en                             :  1 ;  /*        [0] */
   } spi_ctrl ;

#define DUSART_SPI_CTRL_TX_EN_MASK                                       0x0001
#define DUSART_SPI_CTRL_TX_DIS_MASK                                      0x0002
#define DUSART_SPI_CTRL_RX_EN_MASK                                       0x0100
#define DUSART_SPI_CTRL_RX_DIS_MASK                                      0x0200

   struct
   {
      unsigned int   rx_len                            :  4 ;  /*    [15:12] */
      unsigned int   rx_slave_sel                      :  4 ;  /*     [11:8] */
      unsigned int   tx_len                            :  4 ;  /*      [7:4] */
      unsigned int   tx_slave_sel                      :  4 ;  /*      [3:0] */
   } spi_frame_ctrl ;

#define DUSART_SPI_FRAME_CTRL_TX_SLAVE_SEL_MASK                          0x000F
#define DUSART_SPI_FRAME_CTRL_TX_LEN_MASK                                0x00F0
#define DUSART_SPI_FRAME_CTRL_RX_SLAVE_SEL_MASK                          0x0F00
#define DUSART_SPI_FRAME_CTRL_RX_LEN_MASK                                0xF000

   unsigned int __fill0 ;                                      /*       FEF3 */

   struct
   {
      unsigned int   __fill0                           :  8 ;  /*     [15:8] */
      unsigned int   tx_data_pol                       :  1 ;  /*        [7] */
      unsigned int   rx_data_pol                       :  1 ;  /*        [6] */
      unsigned int   stop_bits                         :  2 ;  /*      [5:4] */
      unsigned int   __fill1                           :  2 ;  /*      [3:2] */
      unsigned int   data_size                         :  2 ;  /*      [1:0] */
   } uart_cfg ;

#define DUSART_UART_CFG_DATA_SIZE_FIVE_BITS                    3
#define DUSART_UART_CFG_DATA_SIZE_SIX_BITS                     2
#define DUSART_UART_CFG_DATA_SIZE_SEVEN_BITS                   1
#define DUSART_UART_CFG_DATA_SIZE_EIGHT_BITS                   0

#define DUSART_UART_CFG_STOP_BITS_TWO                          2
#define DUSART_UART_CFG_STOP_BITS_ONE_AND_HALF                 1
#define DUSART_UART_CFG_STOP_BITS_ONE                          0

#define DUSART_UART_CFG_RX_DATA_POL_ACT_HIGH                   1
#define DUSART_UART_CFG_RX_DATA_POL_ACT_LOW                    0

#define DUSART_UART_CFG_TX_DATA_POL_ACT_HIGH                   1
#define DUSART_UART_CFG_TX_DATA_POL_ACT_LOW                    0

#define DUSART_UART_CFG_DATA_SIZE_MASK                                   0x0003
#define DUSART_UART_CFG_STOP_BITS_MASK                                   0x0030
#define DUSART_UART_CFG_RX_DATA_POL_MASK                                 0x0040
#define DUSART_UART_CFG_TX_DATA_POL_MASK                                 0x0080

   struct
   {
      unsigned int   __fill0                           : 10 ;  /*     [15:6] */
      unsigned int   tx_brk_clr                        :  1 ;  /*        [5] */
      unsigned int   tx_brk_set                        :  1 ;  /*        [4] */
      unsigned int   rx_dis                            :  1 ;  /*        [3] */
      unsigned int   rx_en                             :  1 ;  /*        [2] */
      unsigned int   tx_dis                            :  1 ;  /*        [1] */
      unsigned int   tx_en                             :  1 ;  /*        [0] */
   } uart_ctrl ;

#define DUSART_UART_CTRL_TX_EN_MASK                                      0x0001
#define DUSART_UART_CTRL_TX_DIS_MASK                                     0x0002
#define DUSART_UART_CTRL_RX_EN_MASK                                      0x0004
#define DUSART_UART_CTRL_RX_DIS_MASK                                     0x0008
#define DUSART_UART_CTRL_TX_BRK_SET_MASK                                 0x0010
#define DUSART_UART_CTRL_TX_BRK_CLR_MASK                                 0x0020

   struct
   {
      unsigned int   __fill0                           : 11 ;  /*     [15:5] */
      unsigned int   start_rx                          :  1 ;  /*        [4] */
      unsigned int   start_tx                          :  1 ;  /*        [3] */
      unsigned int   pwr_dn                            :  1 ;  /*        [2] */
      unsigned int   pwr_up                            :  1 ;  /*        [1] */
      unsigned int   en                                :  1 ;  /*        [0] */
   } sc_ctrl ;

#define DUSART_SC_CTRL_EN_MASK                                           0x0001
#define DUSART_SC_CTRL_PWR_UP_MASK                                       0x0002
#define DUSART_SC_CTRL_PWR_DN_MASK                                       0x0004
#define DUSART_SC_CTRL_START_TX_MASK                                     0x0008
#define DUSART_SC_CTRL_START_RX_MASK                                     0x0010

   struct
   {
      unsigned int   __fill0                           :  7 ;  /*     [15:9] */
      unsigned int   card_out                          :  1 ;  /*        [8] */
      unsigned int   card_in                           :  1 ;  /*        [7] */
      unsigned int   rst_done                          :  1 ;  /*        [6] */
      unsigned int   sc_pwr_dn                         :  1 ;  /*        [5] */
      unsigned int   sc_pwr_up                         :  1 ;  /*        [4] */
      unsigned int   tx_err                            :  1 ;  /*        [3] */
      unsigned int   tx_grd_done                       :  1 ;  /*        [2] */
      unsigned int   tx_done                           :  1 ;  /*        [1] */
      unsigned int   tx_rdy                            :  1 ;  /*        [0] */
   } sc_sts ;

#define DUSART_SC_STS_TX_RDY_MASK                                        0x0001
#define DUSART_SC_STS_TX_DONE_MASK                                       0x0002
#define DUSART_SC_STS_TX_GRD_DONE_MASK                                   0x0004
#define DUSART_SC_STS_TX_ERR_MASK                                        0x0008
#define DUSART_SC_STS_SC_PWR_UP_MASK                                     0x0010
#define DUSART_SC_STS_SC_PWR_DN_MASK                                     0x0020
#define DUSART_SC_STS_RST_DONE_MASK                                      0x0040
#define DUSART_SC_STS_CARD_IN_MASK                                       0x0080
#define DUSART_SC_STS_CARD_OUT_MASK                                      0x0100

   struct
   {
      unsigned int   __fill0                           :  7 ;  /*     [15:9] */
      unsigned int   card_out                          :  1 ;  /*        [8] */
      unsigned int   card_in                           :  1 ;  /*        [7] */
      unsigned int   rst_done                          :  1 ;  /*        [6] */
      unsigned int   sc_pwr_done                       :  1 ;  /*        [5] */
      unsigned int   sc_pwr_up                         :  1 ;  /*        [4] */
      unsigned int   tx_err                            :  1 ;  /*        [3] */
      unsigned int   tx_grd_done                       :  1 ;  /*        [2] */
      unsigned int   tx_done                           :  1 ;  /*        [1] */
      unsigned int   tx_rdy                            :  1 ;  /*        [0] */
   } sc_int_en ;

#define DUSART_SC_INT_EN_TX_RDY_MASK                                     0x0001
#define DUSART_SC_INT_EN_TX_DONE_MASK                                    0x0002
#define DUSART_SC_INT_EN_TX_GRD_DONE_MASK                                0x0004
#define DUSART_SC_INT_EN_TX_ERR_MASK                                     0x0008
#define DUSART_SC_INT_EN_SC_PWR_UP_MASK                                  0x0010
#define DUSART_SC_INT_EN_SC_PWR_DONE_MASK                                0x0020
#define DUSART_SC_INT_EN_RST_DONE_MASK                                   0x0040
#define DUSART_SC_INT_EN_CARD_IN_MASK                                    0x0080
#define DUSART_SC_INT_EN_CARD_OUT_MASK                                   0x0100

   struct
   {
      unsigned int   __fill0                           :  7 ;  /*     [15:9] */
      unsigned int   card_out                          :  1 ;  /*        [8] */
      unsigned int   card_in                           :  1 ;  /*        [7] */
      unsigned int   rst_done                          :  1 ;  /*        [6] */
      unsigned int   sc_pwr_done                       :  1 ;  /*        [5] */
      unsigned int   sc_pwr_up                         :  1 ;  /*        [4] */
      unsigned int   tx_err                            :  1 ;  /*        [3] */
      unsigned int   tx_grd_done                       :  1 ;  /*        [2] */
      unsigned int   tx_done                           :  1 ;  /*        [1] */
      unsigned int   tx_rdy                            :  1 ;  /*        [0] */
   } sc_int_dis ;

#define DUSART_SC_INT_DIS_TX_RDY_MASK                                    0x0001
#define DUSART_SC_INT_DIS_TX_DONE_MASK                                   0x0002
#define DUSART_SC_INT_DIS_TX_GRD_DONE_MASK                               0x0004
#define DUSART_SC_INT_DIS_TX_ERR_MASK                                    0x0008
#define DUSART_SC_INT_DIS_SC_PWR_UP_MASK                                 0x0010
#define DUSART_SC_INT_DIS_SC_PWR_DONE_MASK                               0x0020
#define DUSART_SC_INT_DIS_RST_DONE_MASK                                  0x0040
#define DUSART_SC_INT_DIS_CARD_IN_MASK                                   0x0080
#define DUSART_SC_INT_DIS_CARD_OUT_MASK                                  0x0100

   struct
   {
      unsigned int   __fill0                           :  7 ;  /*     [15:9] */
      unsigned int   card_out                          :  1 ;  /*        [8] */
      unsigned int   card_in                           :  1 ;  /*        [7] */
      unsigned int   rst_done                          :  1 ;  /*        [6] */
      unsigned int   sc_pwr_done                       :  1 ;  /*        [5] */
      unsigned int   sc_pwr_up                         :  1 ;  /*        [4] */
      unsigned int   tx_err                            :  1 ;  /*        [3] */
      unsigned int   tx_grd_done                       :  1 ;  /*        [2] */
      unsigned int   tx_done                           :  1 ;  /*        [1] */
      unsigned int   tx_rdy                            :  1 ;  /*        [0] */
   } sc_int_clr ;

#define DUSART_SC_INT_CLR_TX_RDY_MASK                                    0x0001
#define DUSART_SC_INT_CLR_TX_DONE_MASK                                   0x0002
#define DUSART_SC_INT_CLR_TX_GRD_DONE_MASK                               0x0004
#define DUSART_SC_INT_CLR_TX_ERR_MASK                                    0x0008
#define DUSART_SC_INT_CLR_SC_PWR_UP_MASK                                 0x0010
#define DUSART_SC_INT_CLR_SC_PWR_DONE_MASK                               0x0020
#define DUSART_SC_INT_CLR_RST_DONE_MASK                                  0x0040
#define DUSART_SC_INT_CLR_CARD_IN_MASK                                   0x0080
#define DUSART_SC_INT_CLR_CARD_OUT_MASK                                  0x0100

   unsigned int __fill1 ;                                      /*       FEFB */

   struct
   {
      unsigned int   __fill0                           :  6 ;  /*    [15:10] */
      unsigned int   card_in_pol                       :  1 ;  /*        [9] */
      unsigned int   clk_on_pol                        :  1 ;  /*        [8] */
      unsigned int   vcc_pol                           :  1 ;  /*        [7] */
      unsigned int   rst_pol                           :  1 ;  /*        [6] */
      unsigned int   data_pol                          :  1 ;  /*        [5] */
      unsigned int   inv_data_pol                      :  1 ;  /*        [4] */
      unsigned int   one_guard_bit                     :  1 ;  /*        [3] */
      unsigned int   auto_deact_en                     :  1 ;  /*        [2] */
      unsigned int   rx_nack_en                        :  1 ;  /*        [1] */
      unsigned int   retx_en                           :  1 ;  /*        [0] */
   } sc_cfg ;

#define DUSART_SC_CFG_RETX_EN_MASK                                       0x0001
#define DUSART_SC_CFG_RX_NACK_EN_MASK                                    0x0002
#define DUSART_SC_CFG_AUTO_DEACT_EN_MASK                                 0x0004
#define DUSART_SC_CFG_ONE_GUARD_BIT_MASK                                 0x0008
#define DUSART_SC_CFG_INV_DATA_POL_MASK                                  0x0010
#define DUSART_SC_CFG_DATA_POL_MASK                                      0x0020
#define DUSART_SC_CFG_RST_POL_MASK                                       0x0040
#define DUSART_SC_CFG_VCC_POL_MASK                                       0x0080
#define DUSART_SC_CFG_CLK_ON_POL_MASK                                    0x0100
#define DUSART_SC_CFG_CARD_IN_POL_MASK                                   0x0200

   struct
   {
      unsigned int   deact_clk_off                     :  8 ;  /*     [15:8] */
      unsigned int   act_vcc_on                        :  8 ;  /*      [7:0] */
   } sc_tim_cfg1 ;

#define DUSART_SC_TIM_CFG1_ACT_VCC_ON_MASK                               0x00FF
#define DUSART_SC_TIM_CFG1_DEACT_CLK_OFF_MASK                            0xFF00

   struct
   {
      unsigned int   rst                               :  8 ;  /*     [15:8] */
      unsigned int   seq_len                           :  8 ;  /*      [7:0] */
   } sc_tim_cfg2 ;

#define DUSART_SC_TIM_CFG2_SEQ_LEN_MASK                                  0x00FF
#define DUSART_SC_TIM_CFG2_RST_MASK                                      0xFF00

   struct
   {
      unsigned int   deact                             :  8 ;  /*     [15:8] */
      unsigned int   guard                             :  8 ;  /*      [7:0] */
   } sc_tim_cfg3 ;

#define DUSART_SC_TIM_CFG3_GUARD_MASK                                    0x00FF
#define DUSART_SC_TIM_CFG3_DEACT_MASK                                    0xFF00

   struct
   {
      unsigned int   __fill0                           : 13 ;  /*     [15:3] */
      unsigned int   start_frame                       :  1 ;  /*        [2] */
      unsigned int   mode                              :  1 ;  /*        [1] */
      unsigned int   ifr_en                            :  1 ;  /*        [0] */
   } ir_ctrl ;

#define DUSART_IR_CTRL_MODE_TX                                 1
#define DUSART_IR_CTRL_MODE_RX                                 0

#define DUSART_IR_CTRL_IFR_EN_MASK                                       0x0001
#define DUSART_IR_CTRL_MODE_MASK                                         0x0002
#define DUSART_IR_CTRL_START_FRAME_MASK                                  0x0004

   struct
   {
      unsigned int   __fill0                           : 12 ;  /*     [15:4] */
      unsigned int   frame_done                        :  1 ;  /*        [3] */
      unsigned int   rx_err                            :  1 ;  /*        [2] */
      unsigned int   rx_done                           :  1 ;  /*        [1] */
      unsigned int   tx_done                           :  1 ;  /*        [0] */
   } ir_sts ;

#define DUSART_IR_STS_TX_DONE_MASK                                       0x0001
#define DUSART_IR_STS_RX_DONE_MASK                                       0x0002
#define DUSART_IR_STS_RX_ERR_MASK                                        0x0004
#define DUSART_IR_STS_FRAME_DONE_MASK                                    0x0008

   struct
   {
      unsigned int   __fill0                           : 12 ;  /*     [15:4] */
      unsigned int   frame_done                        :  1 ;  /*        [3] */
      unsigned int   rx_err                            :  1 ;  /*        [2] */
      unsigned int   rx_done                           :  1 ;  /*        [1] */
      unsigned int   tx_done                           :  1 ;  /*        [0] */
   } ir_int_en ;

#define DUSART_IR_INT_EN_TX_DONE_MASK                                    0x0001
#define DUSART_IR_INT_EN_RX_DONE_MASK                                    0x0002
#define DUSART_IR_INT_EN_RX_ERR_MASK                                     0x0004
#define DUSART_IR_INT_EN_FRAME_DONE_MASK                                 0x0008

   struct
   {
      unsigned int   __fill0                           : 12 ;  /*     [15:4] */
      unsigned int   frame_done                        :  1 ;  /*        [3] */
      unsigned int   rx_err                            :  1 ;  /*        [2] */
      unsigned int   rx_done                           :  1 ;  /*        [1] */
      unsigned int   tx_done                           :  1 ;  /*        [0] */
   } ir_int_dis ;

#define DUSART_IR_INT_DIS_TX_DONE_MASK                                   0x0001
#define DUSART_IR_INT_DIS_RX_DONE_MASK                                   0x0002
#define DUSART_IR_INT_DIS_RX_ERR_MASK                                    0x0004
#define DUSART_IR_INT_DIS_FRAME_DONE_MASK                                0x0008

   struct
   {
      unsigned int   __fill0                           : 12 ;  /*     [15:4] */
      unsigned int   frame_done                        :  1 ;  /*        [3] */
      unsigned int   rx_err                            :  1 ;  /*        [2] */
      unsigned int   rx_done                           :  1 ;  /*        [1] */
      unsigned int   tx_done                           :  1 ;  /*        [0] */
   } ir_int_clr ;

#define DUSART_IR_INT_CLR_TX_DONE_MASK                                   0x0001
#define DUSART_IR_INT_CLR_RX_DONE_MASK                                   0x0002
#define DUSART_IR_INT_CLR_RX_ERR_MASK                                    0x0004
#define DUSART_IR_INT_CLR_FRAME_DONE_MASK                                0x0008

   unsigned int __fill2 ;                                      /*       FF05 */

   struct
   {
      unsigned int   b_pol                             :  1 ;  /*       [15] */
      unsigned int   b_sym                             :  7 ;  /*     [14:8] */
      unsigned int   a_pol                             :  1 ;  /*        [7] */
      unsigned int   a_sym                             :  7 ;  /*      [6:0] */
   } ir_ldin_cfg ;

#define DUSART_IR_LDIN_CFG_A_SYM_MASK                                    0x007F
#define DUSART_IR_LDIN_CFG_A_POL_MASK                                    0x0080
#define DUSART_IR_LDIN_CFG_B_SYM_MASK                                    0x7F00
#define DUSART_IR_LDIN_CFG_B_POL_MASK                                    0x8000

   struct
   {
      unsigned int   b_pol                             :  1 ;  /*       [15] */
      unsigned int   b_sym                             :  7 ;  /*     [14:8] */
      unsigned int   a_pol                             :  1 ;  /*        [7] */
      unsigned int   a_sym                             :  7 ;  /*      [6:0] */
   } ir_d0_cfg ;

#define DUSART_IR_D0_CFG_A_SYM_MASK                                      0x007F
#define DUSART_IR_D0_CFG_A_POL_MASK                                      0x0080
#define DUSART_IR_D0_CFG_B_SYM_MASK                                      0x7F00
#define DUSART_IR_D0_CFG_B_POL_MASK                                      0x8000

   struct
   {
      unsigned int   b_pol                             :  1 ;  /*       [15] */
      unsigned int   b_sym                             :  7 ;  /*     [14:8] */
      unsigned int   a_pol                             :  1 ;  /*        [7] */
      unsigned int   a_sym                             :  7 ;  /*      [6:0] */
   } ir_d1_cfg ;

#define DUSART_IR_D1_CFG_A_SYM_MASK                                      0x007F
#define DUSART_IR_D1_CFG_A_POL_MASK                                      0x0080
#define DUSART_IR_D1_CFG_B_SYM_MASK                                      0x7F00
#define DUSART_IR_D1_CFG_B_POL_MASK                                      0x8000

   struct
   {
      unsigned int   b_pol                             :  1 ;  /*       [15] */
      unsigned int   b_sym                             :  7 ;  /*     [14:8] */
      unsigned int   a_pol                             :  1 ;  /*        [7] */
      unsigned int   a_sym                             :  7 ;  /*      [6:0] */
   } ir_ldout_cfg ;

#define DUSART_IR_LDOUT_CFG_A_SYM_MASK                                   0x007F
#define DUSART_IR_LDOUT_CFG_A_POL_MASK                                   0x0080
#define DUSART_IR_LDOUT_CFG_B_SYM_MASK                                   0x7F00
#define DUSART_IR_LDOUT_CFG_B_POL_MASK                                   0x8000

   struct
   {
      unsigned int   __fill0                           :  1 ;  /*       [15] */
      unsigned int   leadout_sym                       :  7 ;  /*     [14:8] */
      unsigned int   __fill1                           :  1 ;  /*        [7] */
      unsigned int   leadin_sym                        :  7 ;  /*      [6:0] */
   } ir_thresh_cfg ;

#define DUSART_IR_THRESH_CFG_LEADIN_SYM_MASK                             0x007F
#define DUSART_IR_THRESH_CFG_LEADOUT_SYM_MASK                            0x7F00

   struct
   {
      unsigned int   __fill0                           :  6 ;  /*    [15:10] */
      unsigned int   sym                               : 10 ;  /*      [9:0] */
   } ir_len_cfg ;

#define DUSART_IR_LEN_CFG_SYM_MASK                                       0x03FF

   struct
   {
      unsigned int   __fill0                           :  7 ;  /*     [15:9] */
      unsigned int   en                                :  1 ;  /*        [8] */
      unsigned int   min_pulse                         :  8 ;  /*      [7:0] */
   } ir_rx_cfg ;

#define DUSART_IR_RX_CFG_MIN_PULSE_MASK                                  0x00FF
#define DUSART_IR_RX_CFG_EN_MASK                                         0x0100

   struct
   {
      unsigned int   __fill0                           :  2 ;  /*    [15:14] */
      unsigned int   bit_sync_en                       :  1 ;  /*       [13] */
      unsigned int   priority                          :  1 ;  /*       [12] */
      unsigned int   sym_init                          :  4 ;  /*     [11:8] */
      unsigned int   sym1                              :  4 ;  /*      [7:4] */
      unsigned int   sym0                              :  4 ;  /*      [3:0] */
   } ir_rx_bit_cfg ;

#define DUSART_IR_RX_BIT_CFG_SYM0_MASK                                   0x000F
#define DUSART_IR_RX_BIT_CFG_SYM1_MASK                                   0x00F0
#define DUSART_IR_RX_BIT_CFG_SYM_INIT_MASK                               0x0F00
#define DUSART_IR_RX_BIT_CFG_PRIORITY_MASK                               0x1000
#define DUSART_IR_RX_BIT_CFG_BIT_SYNC_EN_MASK                            0x2000

   struct
   {
      unsigned int   __fill0                           :  2 ;  /*    [15:14] */
      unsigned int   mask                              :  6 ;  /*     [13:8] */
      unsigned int   __fill1                           :  2 ;  /*      [7:6] */
      unsigned int   match                             :  6 ;  /*      [5:0] */
   } ir_rx_d0_cfg ;

#define DUSART_IR_RX_D0_CFG_MATCH_MASK                                   0x003F
#define DUSART_IR_RX_D0_CFG_MASK_MASK                                    0x3F00

   struct
   {
      unsigned int   __fill0                           :  2 ;  /*    [15:14] */
      unsigned int   mask                              :  6 ;  /*     [13:8] */
      unsigned int   __fill1                           :  2 ;  /*      [7:6] */
      unsigned int   match                             :  6 ;  /*      [5:0] */
   } ir_rx_d1_cfg ;

#define DUSART_IR_RX_D1_CFG_MATCH_MASK                                   0x003F
#define DUSART_IR_RX_D1_CFG_MASK_MASK                                    0x3F00

   struct
   {
      unsigned int   bit                               :  8 ;  /*     [15:8] */
      unsigned int   leadout                           :  4 ;  /*      [7:4] */
      unsigned int   leadin                            :  4 ;  /*      [3:0] */
   } ir_frame_cfg ;

#define DUSART_IR_FRAME_CFG_LEADIN_MASK                                  0x000F
#define DUSART_IR_FRAME_CFG_LEADOUT_MASK                                 0x00F0
#define DUSART_IR_FRAME_CFG_BIT_MASK                                     0xFF00

   struct
   {
      unsigned int   __fill0                           :  2 ;  /*    [15:14] */
      unsigned int   tx_clk_stall                      :  1 ;  /*       [13] */
      unsigned int   tx_sync                           :  1 ;  /*       [12] */
      unsigned int   tx_sync_sync                      :  1 ;  /*       [11] */
      unsigned int   tx_sym_strb                       :  1 ;  /*       [10] */
      unsigned int   tx_clk                            :  1 ;  /*        [9] */
      unsigned int   tx_cnt                            :  1 ;  /*        [8] */
      unsigned int   __fill1                           :  2 ;  /*      [7:6] */
      unsigned int   rx_clk_stall                      :  1 ;  /*        [5] */
      unsigned int   rx_sync                           :  1 ;  /*        [4] */
      unsigned int   rx_sync_sync                      :  1 ;  /*        [3] */
      unsigned int   rx_sym_strb                       :  1 ;  /*        [2] */
      unsigned int   rx_clk                            :  1 ;  /*        [1] */
      unsigned int   rx_cnt                            :  1 ;  /*        [0] */
   } usr_a_en ;

#define DUSART_USR_A_EN_RX_CNT_MASK                                      0x0001
#define DUSART_USR_A_EN_RX_CLK_MASK                                      0x0002
#define DUSART_USR_A_EN_RX_SYM_STRB_MASK                                 0x0004
#define DUSART_USR_A_EN_RX_SYNC_SYNC_MASK                                0x0008
#define DUSART_USR_A_EN_RX_SYNC_MASK                                     0x0010
#define DUSART_USR_A_EN_RX_CLK_STALL_MASK                                0x0020
#define DUSART_USR_A_EN_TX_CNT_MASK                                      0x0100
#define DUSART_USR_A_EN_TX_CLK_MASK                                      0x0200
#define DUSART_USR_A_EN_TX_SYM_STRB_MASK                                 0x0400
#define DUSART_USR_A_EN_TX_SYNC_SYNC_MASK                                0x0800
#define DUSART_USR_A_EN_TX_SYNC_MASK                                     0x1000
#define DUSART_USR_A_EN_TX_CLK_STALL_MASK                                0x2000

   struct
   {
      unsigned int   __fill0                           :  2 ;  /*    [15:14] */
      unsigned int   tx_clk_stall                      :  1 ;  /*       [13] */
      unsigned int   tx_sync                           :  1 ;  /*       [12] */
      unsigned int   tx_sync_sync                      :  1 ;  /*       [11] */
      unsigned int   tx_sym_strb                       :  1 ;  /*       [10] */
      unsigned int   tx_clk                            :  1 ;  /*        [9] */
      unsigned int   tx_cnt                            :  1 ;  /*        [8] */
      unsigned int   __fill1                           :  2 ;  /*      [7:6] */
      unsigned int   rx_clk_stall                      :  1 ;  /*        [5] */
      unsigned int   rx_sync                           :  1 ;  /*        [4] */
      unsigned int   rx_sync_sync                      :  1 ;  /*        [3] */
      unsigned int   rx_sym_strb                       :  1 ;  /*        [2] */
      unsigned int   rx_clk                            :  1 ;  /*        [1] */
      unsigned int   rx_cnt                            :  1 ;  /*        [0] */
   } usr_a_dis ;

#define DUSART_USR_A_DIS_RX_CNT_MASK                                     0x0001
#define DUSART_USR_A_DIS_RX_CLK_MASK                                     0x0002
#define DUSART_USR_A_DIS_RX_SYM_STRB_MASK                                0x0004
#define DUSART_USR_A_DIS_RX_SYNC_SYNC_MASK                               0x0008
#define DUSART_USR_A_DIS_RX_SYNC_MASK                                    0x0010
#define DUSART_USR_A_DIS_RX_CLK_STALL_MASK                               0x0020
#define DUSART_USR_A_DIS_TX_CNT_MASK                                     0x0100
#define DUSART_USR_A_DIS_TX_CLK_MASK                                     0x0200
#define DUSART_USR_A_DIS_TX_SYM_STRB_MASK                                0x0400
#define DUSART_USR_A_DIS_TX_SYNC_SYNC_MASK                               0x0800
#define DUSART_USR_A_DIS_TX_SYNC_MASK                                    0x1000
#define DUSART_USR_A_DIS_TX_CLK_STALL_MASK                               0x2000

   struct
   {
      unsigned int   __fill0                           :  2 ;  /*    [15:14] */
      unsigned int   tx_clk_stall                      :  1 ;  /*       [13] */
      unsigned int   tx_sync                           :  1 ;  /*       [12] */
      unsigned int   tx_sync_sync                      :  1 ;  /*       [11] */
      unsigned int   tx_sym_strb                       :  1 ;  /*       [10] */
      unsigned int   tx_clk                            :  1 ;  /*        [9] */
      unsigned int   tx_cnt                            :  1 ;  /*        [8] */
      unsigned int   __fill1                           :  2 ;  /*      [7:6] */
      unsigned int   rx_clk_stall                      :  1 ;  /*        [5] */
      unsigned int   rx_sync                           :  1 ;  /*        [4] */
      unsigned int   rx_sync_sync                      :  1 ;  /*        [3] */
      unsigned int   rx_sym_strb                       :  1 ;  /*        [2] */
      unsigned int   rx_clk                            :  1 ;  /*        [1] */
      unsigned int   rx_cnt                            :  1 ;  /*        [0] */
   } usr_b_en ;

#define DUSART_USR_B_EN_RX_CNT_MASK                                      0x0001
#define DUSART_USR_B_EN_RX_CLK_MASK                                      0x0002
#define DUSART_USR_B_EN_RX_SYM_STRB_MASK                                 0x0004
#define DUSART_USR_B_EN_RX_SYNC_SYNC_MASK                                0x0008
#define DUSART_USR_B_EN_RX_SYNC_MASK                                     0x0010
#define DUSART_USR_B_EN_RX_CLK_STALL_MASK                                0x0020
#define DUSART_USR_B_EN_TX_CNT_MASK                                      0x0100
#define DUSART_USR_B_EN_TX_CLK_MASK                                      0x0200
#define DUSART_USR_B_EN_TX_SYM_STRB_MASK                                 0x0400
#define DUSART_USR_B_EN_TX_SYNC_SYNC_MASK                                0x0800
#define DUSART_USR_B_EN_TX_SYNC_MASK                                     0x1000
#define DUSART_USR_B_EN_TX_CLK_STALL_MASK                                0x2000

   struct
   {
      unsigned int   __fill0                           :  2 ;  /*    [15:14] */
      unsigned int   tx_clk_stall                      :  1 ;  /*       [13] */
      unsigned int   tx_sync                           :  1 ;  /*       [12] */
      unsigned int   tx_sync_sync                      :  1 ;  /*       [11] */
      unsigned int   tx_sym_strb                       :  1 ;  /*       [10] */
      unsigned int   tx_clk                            :  1 ;  /*        [9] */
      unsigned int   tx_cnt                            :  1 ;  /*        [8] */
      unsigned int   __fill1                           :  2 ;  /*      [7:6] */
      unsigned int   rx_clk_stall                      :  1 ;  /*        [5] */
      unsigned int   rx_sync                           :  1 ;  /*        [4] */
      unsigned int   rx_sync_sync                      :  1 ;  /*        [3] */
      unsigned int   rx_sym_strb                       :  1 ;  /*        [2] */
      unsigned int   rx_clk                            :  1 ;  /*        [1] */
      unsigned int   rx_cnt                            :  1 ;  /*        [0] */
   } usr_b_dis ;

#define DUSART_USR_B_DIS_RX_CNT_MASK                                     0x0001
#define DUSART_USR_B_DIS_RX_CLK_MASK                                     0x0002
#define DUSART_USR_B_DIS_RX_SYM_STRB_MASK                                0x0004
#define DUSART_USR_B_DIS_RX_SYNC_SYNC_MASK                               0x0008
#define DUSART_USR_B_DIS_RX_SYNC_MASK                                    0x0010
#define DUSART_USR_B_DIS_RX_CLK_STALL_MASK                               0x0020
#define DUSART_USR_B_DIS_TX_CNT_MASK                                     0x0100
#define DUSART_USR_B_DIS_TX_CLK_MASK                                     0x0200
#define DUSART_USR_B_DIS_TX_SYM_STRB_MASK                                0x0400
#define DUSART_USR_B_DIS_TX_SYNC_SYNC_MASK                               0x0800
#define DUSART_USR_B_DIS_TX_SYNC_MASK                                    0x1000
#define DUSART_USR_B_DIS_TX_CLK_STALL_MASK                               0x2000

   struct
   {
      unsigned int   __fill0                           :  2 ;  /*    [15:14] */
      unsigned int   tx_sym_strb                       :  1 ;  /*       [13] */
      unsigned int   tx_sync_strb                      :  1 ;  /*       [12] */
      unsigned int   tx_sr_clr                         :  1 ;  /*       [11] */
      unsigned int   tx_sr_wr                          :  1 ;  /*       [10] */
      unsigned int   tx_par_clr                        :  1 ;  /*        [9] */
      unsigned int   tx_cnt_clr                        :  1 ;  /*        [8] */
      unsigned int   __fill1                           :  2 ;  /*      [7:6] */
      unsigned int   rx_sym_strb                       :  1 ;  /*        [5] */
      unsigned int   rx_sync_strb                      :  1 ;  /*        [4] */
      unsigned int   rx_sr_clr                         :  1 ;  /*        [3] */
      unsigned int   rx_sr_rd                          :  1 ;  /*        [2] */
      unsigned int   rx_par_clr                        :  1 ;  /*        [1] */
      unsigned int   rx_cnt_clr                        :  1 ;  /*        [0] */
   } usr_a_cmd ;

#define DUSART_USR_A_CMD_RX_CNT_CLR_MASK                                 0x0001
#define DUSART_USR_A_CMD_RX_PAR_CLR_MASK                                 0x0002
#define DUSART_USR_A_CMD_RX_SR_RD_MASK                                   0x0004
#define DUSART_USR_A_CMD_RX_SR_CLR_MASK                                  0x0008
#define DUSART_USR_A_CMD_RX_SYNC_STRB_MASK                               0x0010
#define DUSART_USR_A_CMD_RX_SYM_STRB_MASK                                0x0020
#define DUSART_USR_A_CMD_TX_CNT_CLR_MASK                                 0x0100
#define DUSART_USR_A_CMD_TX_PAR_CLR_MASK                                 0x0200
#define DUSART_USR_A_CMD_TX_SR_WR_MASK                                   0x0400
#define DUSART_USR_A_CMD_TX_SR_CLR_MASK                                  0x0800
#define DUSART_USR_A_CMD_TX_SYNC_STRB_MASK                               0x1000
#define DUSART_USR_A_CMD_TX_SYM_STRB_MASK                                0x2000

   struct
   {
      unsigned int   __fill0                           :  2 ;  /*    [15:14] */
      unsigned int   tx_sym_strb                       :  1 ;  /*       [13] */
      unsigned int   tx_sync_strb                      :  1 ;  /*       [12] */
      unsigned int   tx_sr_clr                         :  1 ;  /*       [11] */
      unsigned int   tx_sr_wr                          :  1 ;  /*       [10] */
      unsigned int   tx_par_clr                        :  1 ;  /*        [9] */
      unsigned int   tx_cnt_clr                        :  1 ;  /*        [8] */
      unsigned int   __fill1                           :  2 ;  /*      [7:6] */
      unsigned int   rx_sym_strb                       :  1 ;  /*        [5] */
      unsigned int   rx_sync_strb                      :  1 ;  /*        [4] */
      unsigned int   rx_sr_clr                         :  1 ;  /*        [3] */
      unsigned int   rx_sr_rd                          :  1 ;  /*        [2] */
      unsigned int   rx_par_clr                        :  1 ;  /*        [1] */
      unsigned int   rx_cnt_clr                        :  1 ;  /*        [0] */
   } usr_b_cmd ;

#define DUSART_USR_B_CMD_RX_CNT_CLR_MASK                                 0x0001
#define DUSART_USR_B_CMD_RX_PAR_CLR_MASK                                 0x0002
#define DUSART_USR_B_CMD_RX_SR_RD_MASK                                   0x0004
#define DUSART_USR_B_CMD_RX_SR_CLR_MASK                                  0x0008
#define DUSART_USR_B_CMD_RX_SYNC_STRB_MASK                               0x0010
#define DUSART_USR_B_CMD_RX_SYM_STRB_MASK                                0x0020
#define DUSART_USR_B_CMD_TX_CNT_CLR_MASK                                 0x0100
#define DUSART_USR_B_CMD_TX_PAR_CLR_MASK                                 0x0200
#define DUSART_USR_B_CMD_TX_SR_WR_MASK                                   0x0400
#define DUSART_USR_B_CMD_TX_SR_CLR_MASK                                  0x0800
#define DUSART_USR_B_CMD_TX_SYNC_STRB_MASK                               0x1000
#define DUSART_USR_B_CMD_TX_SYM_STRB_MASK                                0x2000

   struct
   {
      unsigned int   __fill0                           :  4 ;  /*    [15:12] */
      unsigned int   last_par                          :  1 ;  /*       [11] */
      unsigned int   tx_clk_src                        :  1 ;  /*       [10] */
      unsigned int   tx_clk_pol                        :  1 ;  /*        [9] */
      unsigned int   tx_mode                           :  1 ;  /*        [8] */
      unsigned int   __fill1                           :  2 ;  /*      [7:6] */
      unsigned int   rx_clk_src                        :  1 ;  /*        [5] */
      unsigned int   rx_clk_pol                        :  1 ;  /*        [4] */
      unsigned int   rx_mode                           :  1 ;  /*        [3] */
      unsigned int   pol2                              :  1 ;  /*        [2] */
      unsigned int   pol1                              :  1 ;  /*        [1] */
      unsigned int   pol0                              :  1 ;  /*        [0] */
   } usr_a_cfg1 ;

#define DUSART_USR_A_CFG1_POL0_MASK                                      0x0001
#define DUSART_USR_A_CFG1_POL1_MASK                                      0x0002
#define DUSART_USR_A_CFG1_POL2_MASK                                      0x0004
#define DUSART_USR_A_CFG1_RX_MODE_MASK                                   0x0008
#define DUSART_USR_A_CFG1_RX_CLK_POL_MASK                                0x0010
#define DUSART_USR_A_CFG1_RX_CLK_SRC_MASK                                0x0020
#define DUSART_USR_A_CFG1_TX_MODE_MASK                                   0x0100
#define DUSART_USR_A_CFG1_TX_CLK_POL_MASK                                0x0200
#define DUSART_USR_A_CFG1_TX_CLK_SRC_MASK                                0x0400
#define DUSART_USR_A_CFG1_LAST_PAR_MASK                                  0x0800

   struct
   {
      unsigned int   __fill0                           :  4 ;  /*    [15:12] */
      unsigned int   last_par                          :  1 ;  /*       [11] */
      unsigned int   tx_clk_src                        :  1 ;  /*       [10] */
      unsigned int   tx_clk_pol                        :  1 ;  /*        [9] */
      unsigned int   tx_mode                           :  1 ;  /*        [8] */
      unsigned int   __fill1                           :  2 ;  /*      [7:6] */
      unsigned int   rx_clk_src                        :  1 ;  /*        [5] */
      unsigned int   rx_clk_pol                        :  1 ;  /*        [4] */
      unsigned int   rx_mode                           :  1 ;  /*        [3] */
      unsigned int   pol2                              :  1 ;  /*        [2] */
      unsigned int   pol1                              :  1 ;  /*        [1] */
      unsigned int   pol0                              :  1 ;  /*        [0] */
   } usr_b_cfg1 ;

#define DUSART_USR_B_CFG1_POL0_MASK                                      0x0001
#define DUSART_USR_B_CFG1_POL1_MASK                                      0x0002
#define DUSART_USR_B_CFG1_POL2_MASK                                      0x0004
#define DUSART_USR_B_CFG1_RX_MODE_MASK                                   0x0008
#define DUSART_USR_B_CFG1_RX_CLK_POL_MASK                                0x0010
#define DUSART_USR_B_CFG1_RX_CLK_SRC_MASK                                0x0020
#define DUSART_USR_B_CFG1_TX_MODE_MASK                                   0x0100
#define DUSART_USR_B_CFG1_TX_CLK_POL_MASK                                0x0200
#define DUSART_USR_B_CFG1_TX_CLK_SRC_MASK                                0x0400
#define DUSART_USR_B_CFG1_LAST_PAR_MASK                                  0x0800

   struct
   {
      unsigned int   __fill0                           :  7 ;  /*     [15:9] */
      unsigned int   tx_def                            :  1 ;  /*        [8] */
      unsigned int   tx_sym                            :  2 ;  /*      [7:6] */
      unsigned int   rx_sym                            :  2 ;  /*      [5:4] */
      unsigned int   ip_edge                           :  2 ;  /*      [3:2] */
      unsigned int   ip                                :  2 ;  /*      [1:0] */
   } usr_a_cfg2 ;

#define DUSART_USR_A_CFG2_IP_DATA2                             3
#define DUSART_USR_A_CFG2_IP_DATA1                             2
#define DUSART_USR_A_CFG2_IP_DATA0                             1

#define DUSART_USR_A_CFG2_IP_EDGE_FALLING                      3
#define DUSART_USR_A_CFG2_IP_EDGE_RISING                       2
#define DUSART_USR_A_CFG2_IP_EDGE_ANY                          1

#define DUSART_USR_A_CFG2_RX_SYM_EXT                           3
#define DUSART_USR_A_CFG2_RX_SYM_CH1                           2
#define DUSART_USR_A_CFG2_RX_SYM_CH0                           1

#define DUSART_USR_A_CFG2_TX_SYM_EXT                           3
#define DUSART_USR_A_CFG2_TX_SYM_CH1                           2
#define DUSART_USR_A_CFG2_TX_SYM_CH0                           1

#define DUSART_USR_A_CFG2_IP_MASK                                        0x0003
#define DUSART_USR_A_CFG2_IP_EDGE_MASK                                   0x000C
#define DUSART_USR_A_CFG2_RX_SYM_MASK                                    0x0030
#define DUSART_USR_A_CFG2_TX_SYM_MASK                                    0x00C0
#define DUSART_USR_A_CFG2_TX_DEF_MASK                                    0x0100

   struct
   {
      unsigned int   __fill0                           :  7 ;  /*     [15:9] */
      unsigned int   tx_def                            :  1 ;  /*        [8] */
      unsigned int   tx_sym                            :  2 ;  /*      [7:6] */
      unsigned int   rx_sym                            :  2 ;  /*      [5:4] */
      unsigned int   ip_edge                           :  2 ;  /*      [3:2] */
      unsigned int   ip                                :  2 ;  /*      [1:0] */
   } usr_b_cfg2 ;

#define DUSART_USR_B_CFG2_IP_DATA2                             3
#define DUSART_USR_B_CFG2_IP_DATA1                             2
#define DUSART_USR_B_CFG2_IP_DATA0                             1

#define DUSART_USR_B_CFG2_IP_EDGE_FALLING                      3
#define DUSART_USR_B_CFG2_IP_EDGE_RISING                       2
#define DUSART_USR_B_CFG2_IP_EDGE_ANY                          1

#define DUSART_USR_B_CFG2_RX_SYM_EXT                           3
#define DUSART_USR_B_CFG2_RX_SYM_CH1                           2
#define DUSART_USR_B_CFG2_RX_SYM_CH0                           1

#define DUSART_USR_B_CFG2_TX_SYM_EXT                           3
#define DUSART_USR_B_CFG2_TX_SYM_CH1                           2
#define DUSART_USR_B_CFG2_TX_SYM_CH0                           1

#define DUSART_USR_B_CFG2_IP_MASK                                        0x0003
#define DUSART_USR_B_CFG2_IP_EDGE_MASK                                   0x000C
#define DUSART_USR_B_CFG2_RX_SYM_MASK                                    0x0030
#define DUSART_USR_B_CFG2_TX_SYM_MASK                                    0x00C0
#define DUSART_USR_B_CFG2_TX_DEF_MASK                                    0x0100

   struct
   {
      unsigned int   tx_match                          :  8 ;  /*     [15:8] */
      unsigned int   rx_match                          :  8 ;  /*      [7:0] */
   } usr_a_cfg3 ;

#define DUSART_USR_A_CFG3_RX_MATCH_MASK                                  0x00FF
#define DUSART_USR_A_CFG3_TX_MATCH_MASK                                  0xFF00

   struct
   {
      unsigned int   tx_match                          :  8 ;  /*     [15:8] */
      unsigned int   rx_match                          :  8 ;  /*      [7:0] */
   } usr_b_cfg3 ;

#define DUSART_USR_B_CFG3_RX_MATCH_MASK                                  0x00FF
#define DUSART_USR_B_CFG3_TX_MATCH_MASK                                  0xFF00

} ;



struct __tim_fd_tag
{
   struct
   {
      unsigned int   __fill0                           :  8 ;  /*     [15:8] */
      unsigned int   ltmr_ld                           :  1 ;  /*        [7] */
      unsigned int   ex_ld                             :  1 ;  /*        [6] */
      unsigned int   cap_cnt_clr                       :  1 ;  /*        [5] */
      unsigned int   pwm2_ld                           :  1 ;  /*        [4] */
      unsigned int   pwm1_ld                           :  1 ;  /*        [3] */
      unsigned int   cnt2_ld                           :  1 ;  /*        [2] */
      unsigned int   cnt1_ld                           :  1 ;  /*        [1] */
      unsigned int   tmr_ld                            :  1 ;  /*        [0] */
   } cmd ;

#define TIM_CMD_TMR_LD_MASK                                              0x0001
#define TIM_CMD_CNT1_LD_MASK                                             0x0002
#define TIM_CMD_CNT2_LD_MASK                                             0x0004
#define TIM_CMD_PWM1_LD_MASK                                             0x0008
#define TIM_CMD_PWM2_LD_MASK                                             0x0010
#define TIM_CMD_CAP_CNT_CLR_MASK                                         0x0020
#define TIM_CMD_EX_LD_MASK                                               0x0040
#define TIM_CMD_LTMR_LD_MASK                                             0x0080

   struct
   {
      unsigned int   ltmr_cnt                          :  1 ;  /*       [15] */
      unsigned int   ex_cnt                            :  1 ;  /*       [14] */
      unsigned int   cap_cnt                           :  1 ;  /*       [13] */
      unsigned int   pwm2_cnt                          :  1 ;  /*       [12] */
      unsigned int   pwm1_cnt                          :  1 ;  /*       [11] */
      unsigned int   cnt2_cnt                          :  1 ;  /*       [10] */
      unsigned int   cnt1_cnt                          :  1 ;  /*        [9] */
      unsigned int   tmr_cnt                           :  1 ;  /*        [8] */
      unsigned int   ltmr_auto_re_ld                   :  1 ;  /*        [7] */
      unsigned int   __fill0                           :  2 ;  /*      [6:5] */
      unsigned int   pwm2_auto_re_ld                   :  1 ;  /*        [4] */
      unsigned int   pwm1_auto_re_ld                   :  1 ;  /*        [3] */
      unsigned int   cnt2_auto_re_ld                   :  1 ;  /*        [2] */
      unsigned int   cnt1_auto_re_ld                   :  1 ;  /*        [1] */
      unsigned int   tmr_auto_re_ld                    :  1 ;  /*        [0] */
   } ctrl_en ;

#define TIM_CTRL_EN_TMR_AUTO_RE_LD_MASK                                  0x0001
#define TIM_CTRL_EN_CNT1_AUTO_RE_LD_MASK                                 0x0002
#define TIM_CTRL_EN_CNT2_AUTO_RE_LD_MASK                                 0x0004
#define TIM_CTRL_EN_PWM1_AUTO_RE_LD_MASK                                 0x0008
#define TIM_CTRL_EN_PWM2_AUTO_RE_LD_MASK                                 0x0010
#define TIM_CTRL_EN_LTMR_AUTO_RE_LD_MASK                                 0x0080
#define TIM_CTRL_EN_TMR_CNT_MASK                                         0x0100
#define TIM_CTRL_EN_CNT1_CNT_MASK                                        0x0200
#define TIM_CTRL_EN_CNT2_CNT_MASK                                        0x0400
#define TIM_CTRL_EN_PWM1_CNT_MASK                                        0x0800
#define TIM_CTRL_EN_PWM2_CNT_MASK                                        0x1000
#define TIM_CTRL_EN_CAP_CNT_MASK                                         0x2000
#define TIM_CTRL_EN_EX_CNT_MASK                                          0x4000
#define TIM_CTRL_EN_LTMR_CNT_MASK                                        0x8000

   struct
   {
      unsigned int   ltmr_cnt                          :  1 ;  /*       [15] */
      unsigned int   ex_cnt                            :  1 ;  /*       [14] */
      unsigned int   cap_cnt                           :  1 ;  /*       [13] */
      unsigned int   pwm2_cnt                          :  1 ;  /*       [12] */
      unsigned int   pwm1_cnt                          :  1 ;  /*       [11] */
      unsigned int   cnt2_cnt                          :  1 ;  /*       [10] */
      unsigned int   cnt1_cnt                          :  1 ;  /*        [9] */
      unsigned int   tmr_cnt                           :  1 ;  /*        [8] */
      unsigned int   ltmr_auto_re_ld                   :  1 ;  /*        [7] */
      unsigned int   __fill0                           :  2 ;  /*      [6:5] */
      unsigned int   pwm2_auto_re_ld                   :  1 ;  /*        [4] */
      unsigned int   pwm1_auto_re_ld                   :  1 ;  /*        [3] */
      unsigned int   cnt2_auto_re_ld                   :  1 ;  /*        [2] */
      unsigned int   cnt1_auto_re_ld                   :  1 ;  /*        [1] */
      unsigned int   tmr_auto_re_ld                    :  1 ;  /*        [0] */
   } ctrl_dis ;

#define TIM_CTRL_DIS_TMR_AUTO_RE_LD_MASK                                 0x0001
#define TIM_CTRL_DIS_CNT1_AUTO_RE_LD_MASK                                0x0002
#define TIM_CTRL_DIS_CNT2_AUTO_RE_LD_MASK                                0x0004
#define TIM_CTRL_DIS_PWM1_AUTO_RE_LD_MASK                                0x0008
#define TIM_CTRL_DIS_PWM2_AUTO_RE_LD_MASK                                0x0010
#define TIM_CTRL_DIS_LTMR_AUTO_RE_LD_MASK                                0x0080
#define TIM_CTRL_DIS_TMR_CNT_MASK                                        0x0100
#define TIM_CTRL_DIS_CNT1_CNT_MASK                                       0x0200
#define TIM_CTRL_DIS_CNT2_CNT_MASK                                       0x0400
#define TIM_CTRL_DIS_PWM1_CNT_MASK                                       0x0800
#define TIM_CTRL_DIS_PWM2_CNT_MASK                                       0x1000
#define TIM_CTRL_DIS_CAP_CNT_MASK                                        0x2000
#define TIM_CTRL_DIS_EX_CNT_MASK                                         0x4000
#define TIM_CTRL_DIS_LTMR_CNT_MASK                                       0x8000

   struct
   {
      unsigned int   tmr_ld                            : 16 ;  /*     [15:0] */
   } tmr_ld ;

#define TIM_TMR_LD_TMR_LD_MASK                                           0xFFFF

   struct
   {
      unsigned int   cnt1_ld                           : 16 ;  /*     [15:0] */
   } cnt1_ld ;

#define TIM_CNT1_LD_CNT1_LD_MASK                                         0xFFFF

   struct
   {
      unsigned int   cnt1_cmp                          : 16 ;  /*     [15:0] */
   } cnt1_cmp ;

#define TIM_CNT1_CMP_CNT1_CMP_MASK                                       0xFFFF

   struct
   {
      unsigned int   __fill0                           : 13 ;  /*     [15:3] */
      unsigned int   ext                               :  1 ;  /*        [2] */
      unsigned int   edge                              :  2 ;  /*      [1:0] */
   } cnt1_cfg ;

#define TIM_CNT1_CFG_EDGE_FALLING                              2
#define TIM_CNT1_CFG_EDGE_RISING                               1
#define TIM_CNT1_CFG_EDGE_BOTH                                 0

#define TIM_CNT1_CFG_EDGE_MASK                                           0x0003
#define TIM_CNT1_CFG_EXT_MASK                                            0x0004

   struct
   {
      unsigned int   cnt2_ld                           : 16 ;  /*     [15:0] */
   } cnt2_ld ;

#define TIM_CNT2_LD_CNT2_LD_MASK                                         0xFFFF

   struct
   {
      unsigned int   cnt2_cmp                          : 16 ;  /*     [15:0] */
   } cnt2_cmp ;

#define TIM_CNT2_CMP_CNT2_CMP_MASK                                       0xFFFF

   struct
   {
      unsigned int   __fill0                           : 13 ;  /*     [15:3] */
      unsigned int   ext                               :  1 ;  /*        [2] */
      unsigned int   edge                              :  2 ;  /*      [1:0] */
   } cnt2_cfg ;

#define TIM_CNT2_CFG_EDGE_FALLING                              2
#define TIM_CNT2_CFG_EDGE_RISING                               1
#define TIM_CNT2_CFG_EDGE_BOTH                                 0

#define TIM_CNT2_CFG_EDGE_MASK                                           0x0003
#define TIM_CNT2_CFG_EXT_MASK                                            0x0004

   struct
   {
      unsigned int   pwm1_ld                           : 16 ;  /*     [15:0] */
   } pwm1_ld ;

#define TIM_PWM1_LD_PWM1_LD_MASK                                         0xFFFF

   struct
   {
      unsigned int   pwm1_val                          : 16 ;  /*     [15:0] */
   } pwm1_val ;

#define TIM_PWM1_VAL_PWM1_VAL_MASK                                       0xFFFF

   struct
   {
      unsigned int   __fill0                           : 13 ;  /*     [15:3] */
      unsigned int   pol                               :  1 ;  /*        [2] */
      unsigned int   hw_reload_polarity                :  1 ;  /*        [1] */
      unsigned int   sw_reload                         :  1 ;  /*        [0] */
   } pwm1_cfg ;

#define TIM_PWM1_CFG_SW_RELOAD_MASK                                      0x0001
#define TIM_PWM1_CFG_HW_RELOAD_POLARITY_MASK                             0x0002
#define TIM_PWM1_CFG_POL_MASK                                            0x0004

   struct
   {
      unsigned int   pwm2_ld                           : 16 ;  /*     [15:0] */
   } pwm2_ld ;

#define TIM_PWM2_LD_PWM2_LD_MASK                                         0xFFFF

   struct
   {
      unsigned int   pwm2_val                          : 16 ;  /*     [15:0] */
   } pwm2_val ;

#define TIM_PWM2_VAL_PWM2_VAL_MASK                                       0xFFFF

   struct
   {
      unsigned int   __fill0                           : 13 ;  /*     [15:3] */
      unsigned int   pol                               :  1 ;  /*        [2] */
      unsigned int   hw_reload_polarity                :  1 ;  /*        [1] */
      unsigned int   sw_reload                         :  1 ;  /*        [0] */
   } pwm2_cfg ;

#define TIM_PWM2_CFG_SW_RELOAD_MASK                                      0x0001
#define TIM_PWM2_CFG_HW_RELOAD_POLARITY_MASK                             0x0002
#define TIM_PWM2_CFG_POL_MASK                                            0x0004

   struct
   {
      unsigned int   __fill0                           :  4 ;  /*    [15:12] */
      unsigned int   edge_cap6                         :  2 ;  /*    [11:10] */
      unsigned int   edge_cap5                         :  2 ;  /*      [9:8] */
      unsigned int   edge_cap4                         :  2 ;  /*      [7:6] */
      unsigned int   edge_cap3                         :  2 ;  /*      [5:4] */
      unsigned int   edge_cap2                         :  2 ;  /*      [3:2] */
      unsigned int   edge_cap1                         :  2 ;  /*      [1:0] */
   } cap_cfg ;

#define TIM_CAP_CFG_EDGE_CAP1_BOTH                             3
#define TIM_CAP_CFG_EDGE_CAP1_FALLING                          2
#define TIM_CAP_CFG_EDGE_CAP1_RISE                             1

#define TIM_CAP_CFG_EDGE_CAP2_BOTH                             3
#define TIM_CAP_CFG_EDGE_CAP2_FALLING                          2
#define TIM_CAP_CFG_EDGE_CAP2_RISE                             1

#define TIM_CAP_CFG_EDGE_CAP3_BOTH                             3
#define TIM_CAP_CFG_EDGE_CAP3_FALLING                          2
#define TIM_CAP_CFG_EDGE_CAP3_RISE                             1

#define TIM_CAP_CFG_EDGE_CAP4_BOTH                             3
#define TIM_CAP_CFG_EDGE_CAP4_FALLING                          2
#define TIM_CAP_CFG_EDGE_CAP4_RISE                             1

#define TIM_CAP_CFG_EDGE_CAP5_BOTH                             3
#define TIM_CAP_CFG_EDGE_CAP5_FALLING                          2
#define TIM_CAP_CFG_EDGE_CAP5_RISE                             1

#define TIM_CAP_CFG_EDGE_CAP6_BOTH                             3
#define TIM_CAP_CFG_EDGE_CAP6_FALLING                          2
#define TIM_CAP_CFG_EDGE_CAP6_RISE                             1

#define TIM_CAP_CFG_EDGE_CAP1_MASK                                       0x0003
#define TIM_CAP_CFG_EDGE_CAP2_MASK                                       0x000C
#define TIM_CAP_CFG_EDGE_CAP3_MASK                                       0x0030
#define TIM_CAP_CFG_EDGE_CAP4_MASK                                       0x00C0
#define TIM_CAP_CFG_EDGE_CAP5_MASK                                       0x0300
#define TIM_CAP_CFG_EDGE_CAP6_MASK                                       0x0C00

   struct
   {
      unsigned int   ex_ld                             : 16 ;  /*     [15:0] */
   } ex_ld ;

#define TIM_EX_LD_EX_LD_MASK                                             0xFFFF

   struct
   {
      unsigned int   ltmr_ld                           : 16 ;  /*     [15:0] */
   } ltmr_ld ;

#define TIM_LTMR_LD_LTMR_LD_MASK                                         0xFFFF

   struct
   {
      unsigned int   tmr_cnt                           : 16 ;  /*     [15:0] */
   } tmr_cnt ;

#define TIM_TMR_CNT_TMR_CNT_MASK                                         0xFFFF

   struct
   {
      unsigned int   cnt1_cnt                          : 16 ;  /*     [15:0] */
   } cnt1_cnt ;

#define TIM_CNT1_CNT_CNT1_CNT_MASK                                       0xFFFF

   struct
   {
      unsigned int   cnt2_cnt                          : 16 ;  /*     [15:0] */
   } cnt2_cnt ;

#define TIM_CNT2_CNT_CNT2_CNT_MASK                                       0xFFFF

   struct
   {
      unsigned int   cap_val1                          : 16 ;  /*     [15:0] */
   } cap_val1 ;

#define TIM_CAP_VAL1_CAP_VAL1_MASK                                       0xFFFF

   struct
   {
      unsigned int   cap_val2                          : 16 ;  /*     [15:0] */
   } cap_val2 ;

#define TIM_CAP_VAL2_CAP_VAL2_MASK                                       0xFFFF

   struct
   {
      unsigned int   cap_val3                          : 16 ;  /*     [15:0] */
   } cap_val3 ;

#define TIM_CAP_VAL3_CAP_VAL3_MASK                                       0xFFFF

   struct
   {
      unsigned int   cap_val4                          : 16 ;  /*     [15:0] */
   } cap_val4 ;

#define TIM_CAP_VAL4_CAP_VAL4_MASK                                       0xFFFF

   struct
   {
      unsigned int   cap_val5                          : 16 ;  /*     [15:0] */
   } cap_val5 ;

#define TIM_CAP_VAL5_CAP_VAL5_MASK                                       0xFFFF

   struct
   {
      unsigned int   cap_val6                          : 16 ;  /*     [15:0] */
   } cap_val6 ;

#define TIM_CAP_VAL6_CAP_VAL6_MASK                                       0xFFFF

   struct
   {
      unsigned int   __fill0                           :  4 ;  /*    [15:12] */
      unsigned int   ltmr_exp                          :  1 ;  /*       [11] */
      unsigned int   ex_exp                            :  1 ;  /*       [10] */
      unsigned int   pwm2_match                        :  1 ;  /*        [9] */
      unsigned int   pwm2_exp                          :  1 ;  /*        [8] */
      unsigned int   pwm1_match                        :  1 ;  /*        [7] */
      unsigned int   pwm1_exp                          :  1 ;  /*        [6] */
      unsigned int   cnt2_match                        :  1 ;  /*        [5] */
      unsigned int   cnt2_exp                          :  1 ;  /*        [4] */
      unsigned int   cnt1_match                        :  1 ;  /*        [3] */
      unsigned int   cnt1_exp                          :  1 ;  /*        [2] */
      unsigned int   __fill1                           :  1 ;  /*        [1] */
      unsigned int   tmr_exp                           :  1 ;  /*        [0] */
   } int_sts1 ;

#define TIM_INT_STS1_TMR_EXP_MASK                                        0x0001
#define TIM_INT_STS1_CNT1_EXP_MASK                                       0x0004
#define TIM_INT_STS1_CNT1_MATCH_MASK                                     0x0008
#define TIM_INT_STS1_CNT2_EXP_MASK                                       0x0010
#define TIM_INT_STS1_CNT2_MATCH_MASK                                     0x0020
#define TIM_INT_STS1_PWM1_EXP_MASK                                       0x0040
#define TIM_INT_STS1_PWM1_MATCH_MASK                                     0x0080
#define TIM_INT_STS1_PWM2_EXP_MASK                                       0x0100
#define TIM_INT_STS1_PWM2_MATCH_MASK                                     0x0200
#define TIM_INT_STS1_EX_EXP_MASK                                         0x0400
#define TIM_INT_STS1_LTMR_EXP_MASK                                       0x0800

   struct
   {
      unsigned int   __fill0                           :  2 ;  /*    [15:14] */
      unsigned int   cap6_ovwr                         :  1 ;  /*       [13] */
      unsigned int   cap6                              :  1 ;  /*       [12] */
      unsigned int   cap5_ovwr                         :  1 ;  /*       [11] */
      unsigned int   cap5                              :  1 ;  /*       [10] */
      unsigned int   cap4_ovwr                         :  1 ;  /*        [9] */
      unsigned int   cap4                              :  1 ;  /*        [8] */
      unsigned int   cap3_ovwr                         :  1 ;  /*        [7] */
      unsigned int   cap3                              :  1 ;  /*        [6] */
      unsigned int   cap2_ovwr                         :  1 ;  /*        [5] */
      unsigned int   cap2                              :  1 ;  /*        [4] */
      unsigned int   cap1_ovwr                         :  1 ;  /*        [3] */
      unsigned int   cap1                              :  1 ;  /*        [2] */
      unsigned int   __fill1                           :  1 ;  /*        [1] */
      unsigned int   cap_exp                           :  1 ;  /*        [0] */
   } int_sts2 ;

#define TIM_INT_STS2_CAP_EXP_MASK                                        0x0001
#define TIM_INT_STS2_CAP1_MASK                                           0x0004
#define TIM_INT_STS2_CAP1_OVWR_MASK                                      0x0008
#define TIM_INT_STS2_CAP2_MASK                                           0x0010
#define TIM_INT_STS2_CAP2_OVWR_MASK                                      0x0020
#define TIM_INT_STS2_CAP3_MASK                                           0x0040
#define TIM_INT_STS2_CAP3_OVWR_MASK                                      0x0080
#define TIM_INT_STS2_CAP4_MASK                                           0x0100
#define TIM_INT_STS2_CAP4_OVWR_MASK                                      0x0200
#define TIM_INT_STS2_CAP5_MASK                                           0x0400
#define TIM_INT_STS2_CAP5_OVWR_MASK                                      0x0800
#define TIM_INT_STS2_CAP6_MASK                                           0x1000
#define TIM_INT_STS2_CAP6_OVWR_MASK                                      0x2000

   struct
   {
      unsigned int   __fill0                           :  4 ;  /*    [15:12] */
      unsigned int   ltmr_exp                          :  1 ;  /*       [11] */
      unsigned int   ex_exp                            :  1 ;  /*       [10] */
      unsigned int   pwm2_match                        :  1 ;  /*        [9] */
      unsigned int   pwm2_exp                          :  1 ;  /*        [8] */
      unsigned int   pwm1_match                        :  1 ;  /*        [7] */
      unsigned int   pwm1_exp                          :  1 ;  /*        [6] */
      unsigned int   cnt2_match                        :  1 ;  /*        [5] */
      unsigned int   cnt2_exp                          :  1 ;  /*        [4] */
      unsigned int   cnt1_match                        :  1 ;  /*        [3] */
      unsigned int   cnt1_exp                          :  1 ;  /*        [2] */
      unsigned int   __fill1                           :  1 ;  /*        [1] */
      unsigned int   tmr_exp                           :  1 ;  /*        [0] */
   } int_en1 ;

#define TIM_INT_EN1_TMR_EXP_MASK                                         0x0001
#define TIM_INT_EN1_CNT1_EXP_MASK                                        0x0004
#define TIM_INT_EN1_CNT1_MATCH_MASK                                      0x0008
#define TIM_INT_EN1_CNT2_EXP_MASK                                        0x0010
#define TIM_INT_EN1_CNT2_MATCH_MASK                                      0x0020
#define TIM_INT_EN1_PWM1_EXP_MASK                                        0x0040
#define TIM_INT_EN1_PWM1_MATCH_MASK                                      0x0080
#define TIM_INT_EN1_PWM2_EXP_MASK                                        0x0100
#define TIM_INT_EN1_PWM2_MATCH_MASK                                      0x0200
#define TIM_INT_EN1_EX_EXP_MASK                                          0x0400
#define TIM_INT_EN1_LTMR_EXP_MASK                                        0x0800

   struct
   {
      unsigned int   __fill0                           :  2 ;  /*    [15:14] */
      unsigned int   cap6_ovwr                         :  1 ;  /*       [13] */
      unsigned int   cap6                              :  1 ;  /*       [12] */
      unsigned int   cap5_ovwr                         :  1 ;  /*       [11] */
      unsigned int   cap5                              :  1 ;  /*       [10] */
      unsigned int   cap4_ovwr                         :  1 ;  /*        [9] */
      unsigned int   cap4                              :  1 ;  /*        [8] */
      unsigned int   cap3_ovwr                         :  1 ;  /*        [7] */
      unsigned int   cap3                              :  1 ;  /*        [6] */
      unsigned int   cap2_ovwr                         :  1 ;  /*        [5] */
      unsigned int   cap2                              :  1 ;  /*        [4] */
      unsigned int   cap1_ovwr                         :  1 ;  /*        [3] */
      unsigned int   cap1                              :  1 ;  /*        [2] */
      unsigned int   __fill1                           :  1 ;  /*        [1] */
      unsigned int   cap_exp                           :  1 ;  /*        [0] */
   } int_en2 ;

#define TIM_INT_EN2_CAP_EXP_MASK                                         0x0001
#define TIM_INT_EN2_CAP1_MASK                                            0x0004
#define TIM_INT_EN2_CAP1_OVWR_MASK                                       0x0008
#define TIM_INT_EN2_CAP2_MASK                                            0x0010
#define TIM_INT_EN2_CAP2_OVWR_MASK                                       0x0020
#define TIM_INT_EN2_CAP3_MASK                                            0x0040
#define TIM_INT_EN2_CAP3_OVWR_MASK                                       0x0080
#define TIM_INT_EN2_CAP4_MASK                                            0x0100
#define TIM_INT_EN2_CAP4_OVWR_MASK                                       0x0200
#define TIM_INT_EN2_CAP5_MASK                                            0x0400
#define TIM_INT_EN2_CAP5_OVWR_MASK                                       0x0800
#define TIM_INT_EN2_CAP6_MASK                                            0x1000
#define TIM_INT_EN2_CAP6_OVWR_MASK                                       0x2000

   struct
   {
      unsigned int   __fill0                           :  4 ;  /*    [15:12] */
      unsigned int   ltmr_exp                          :  1 ;  /*       [11] */
      unsigned int   ex_exp                            :  1 ;  /*       [10] */
      unsigned int   pwm2_match                        :  1 ;  /*        [9] */
      unsigned int   pwm2_exp                          :  1 ;  /*        [8] */
      unsigned int   pwm1_match                        :  1 ;  /*        [7] */
      unsigned int   pwm1_exp                          :  1 ;  /*        [6] */
      unsigned int   cnt2_match                        :  1 ;  /*        [5] */
      unsigned int   cnt2_exp                          :  1 ;  /*        [4] */
      unsigned int   cnt1_match                        :  1 ;  /*        [3] */
      unsigned int   cnt1_exp                          :  1 ;  /*        [2] */
      unsigned int   __fill1                           :  1 ;  /*        [1] */
      unsigned int   tmr_exp                           :  1 ;  /*        [0] */
   } int_dis1 ;

#define TIM_INT_DIS1_TMR_EXP_MASK                                        0x0001
#define TIM_INT_DIS1_CNT1_EXP_MASK                                       0x0004
#define TIM_INT_DIS1_CNT1_MATCH_MASK                                     0x0008
#define TIM_INT_DIS1_CNT2_EXP_MASK                                       0x0010
#define TIM_INT_DIS1_CNT2_MATCH_MASK                                     0x0020
#define TIM_INT_DIS1_PWM1_EXP_MASK                                       0x0040
#define TIM_INT_DIS1_PWM1_MATCH_MASK                                     0x0080
#define TIM_INT_DIS1_PWM2_EXP_MASK                                       0x0100
#define TIM_INT_DIS1_PWM2_MATCH_MASK                                     0x0200
#define TIM_INT_DIS1_EX_EXP_MASK                                         0x0400
#define TIM_INT_DIS1_LTMR_EXP_MASK                                       0x0800

   struct
   {
      unsigned int   __fill0                           :  2 ;  /*    [15:14] */
      unsigned int   cap6_ovwr                         :  1 ;  /*       [13] */
      unsigned int   cap6                              :  1 ;  /*       [12] */
      unsigned int   cap5_ovwr                         :  1 ;  /*       [11] */
      unsigned int   cap5                              :  1 ;  /*       [10] */
      unsigned int   cap4_ovwr                         :  1 ;  /*        [9] */
      unsigned int   cap4                              :  1 ;  /*        [8] */
      unsigned int   cap3_ovwr                         :  1 ;  /*        [7] */
      unsigned int   cap3                              :  1 ;  /*        [6] */
      unsigned int   cap2_ovwr                         :  1 ;  /*        [5] */
      unsigned int   cap2                              :  1 ;  /*        [4] */
      unsigned int   cap1_ovwr                         :  1 ;  /*        [3] */
      unsigned int   cap1                              :  1 ;  /*        [2] */
      unsigned int   __fill1                           :  1 ;  /*        [1] */
      unsigned int   cap_exp                           :  1 ;  /*        [0] */
   } int_dis2 ;

#define TIM_INT_DIS2_CAP_EXP_MASK                                        0x0001
#define TIM_INT_DIS2_CAP1_MASK                                           0x0004
#define TIM_INT_DIS2_CAP1_OVWR_MASK                                      0x0008
#define TIM_INT_DIS2_CAP2_MASK                                           0x0010
#define TIM_INT_DIS2_CAP2_OVWR_MASK                                      0x0020
#define TIM_INT_DIS2_CAP3_MASK                                           0x0040
#define TIM_INT_DIS2_CAP3_OVWR_MASK                                      0x0080
#define TIM_INT_DIS2_CAP4_MASK                                           0x0100
#define TIM_INT_DIS2_CAP4_OVWR_MASK                                      0x0200
#define TIM_INT_DIS2_CAP5_MASK                                           0x0400
#define TIM_INT_DIS2_CAP5_OVWR_MASK                                      0x0800
#define TIM_INT_DIS2_CAP6_MASK                                           0x1000
#define TIM_INT_DIS2_CAP6_OVWR_MASK                                      0x2000

   struct
   {
      unsigned int   __fill0                           :  4 ;  /*    [15:12] */
      unsigned int   ltmr_exp                          :  1 ;  /*       [11] */
      unsigned int   ex_exp                            :  1 ;  /*       [10] */
      unsigned int   pwm2_match                        :  1 ;  /*        [9] */
      unsigned int   pwm2_exp                          :  1 ;  /*        [8] */
      unsigned int   pwm1_match                        :  1 ;  /*        [7] */
      unsigned int   pwm1_exp                          :  1 ;  /*        [6] */
      unsigned int   cnt2_match                        :  1 ;  /*        [5] */
      unsigned int   cnt2_exp                          :  1 ;  /*        [4] */
      unsigned int   cnt1_match                        :  1 ;  /*        [3] */
      unsigned int   cnt1_exp                          :  1 ;  /*        [2] */
      unsigned int   __fill1                           :  1 ;  /*        [1] */
      unsigned int   tmr_exp                           :  1 ;  /*        [0] */
   } int_clr1 ;

#define TIM_INT_CLR1_TMR_EXP_MASK                                        0x0001
#define TIM_INT_CLR1_CNT1_EXP_MASK                                       0x0004
#define TIM_INT_CLR1_CNT1_MATCH_MASK                                     0x0008
#define TIM_INT_CLR1_CNT2_EXP_MASK                                       0x0010
#define TIM_INT_CLR1_CNT2_MATCH_MASK                                     0x0020
#define TIM_INT_CLR1_PWM1_EXP_MASK                                       0x0040
#define TIM_INT_CLR1_PWM1_MATCH_MASK                                     0x0080
#define TIM_INT_CLR1_PWM2_EXP_MASK                                       0x0100
#define TIM_INT_CLR1_PWM2_MATCH_MASK                                     0x0200
#define TIM_INT_CLR1_EX_EXP_MASK                                         0x0400
#define TIM_INT_CLR1_LTMR_EXP_MASK                                       0x0800

   struct
   {
      unsigned int   __fill0                           :  2 ;  /*    [15:14] */
      unsigned int   cap6_ovwr                         :  1 ;  /*       [13] */
      unsigned int   cap6                              :  1 ;  /*       [12] */
      unsigned int   cap5_ovwr                         :  1 ;  /*       [11] */
      unsigned int   cap5                              :  1 ;  /*       [10] */
      unsigned int   cap4_ovwr                         :  1 ;  /*        [9] */
      unsigned int   cap4                              :  1 ;  /*        [8] */
      unsigned int   cap3_ovwr                         :  1 ;  /*        [7] */
      unsigned int   cap3                              :  1 ;  /*        [6] */
      unsigned int   cap2_ovwr                         :  1 ;  /*        [5] */
      unsigned int   cap2                              :  1 ;  /*        [4] */
      unsigned int   cap1_ovwr                         :  1 ;  /*        [3] */
      unsigned int   cap1                              :  1 ;  /*        [2] */
      unsigned int   __fill1                           :  1 ;  /*        [1] */
      unsigned int   cap_exp                           :  1 ;  /*        [0] */
   } int_clr2 ;

#define TIM_INT_CLR2_CAP_EXP_MASK                                        0x0001
#define TIM_INT_CLR2_CAP1_MASK                                           0x0004
#define TIM_INT_CLR2_CAP1_OVWR_MASK                                      0x0008
#define TIM_INT_CLR2_CAP2_MASK                                           0x0010
#define TIM_INT_CLR2_CAP2_OVWR_MASK                                      0x0020
#define TIM_INT_CLR2_CAP3_MASK                                           0x0040
#define TIM_INT_CLR2_CAP3_OVWR_MASK                                      0x0080
#define TIM_INT_CLR2_CAP4_MASK                                           0x0100
#define TIM_INT_CLR2_CAP4_OVWR_MASK                                      0x0200
#define TIM_INT_CLR2_CAP5_MASK                                           0x0400
#define TIM_INT_CLR2_CAP5_OVWR_MASK                                      0x0800
#define TIM_INT_CLR2_CAP6_MASK                                           0x1000
#define TIM_INT_CLR2_CAP6_OVWR_MASK                                      0x2000

} ;



struct __cache_fd_tag
{
   struct
   {
      unsigned int   __fill0                           :  5 ;  /*    [15:11] */
      unsigned int   waits                             :  3 ;  /*     [10:8] */
      unsigned int   wr_sel                            :  2 ;  /*      [7:6] */
      unsigned int   __fill1                           :  1 ;  /*        [5] */
      unsigned int   lk_bit_en                         :  1 ;  /*        [4] */
      unsigned int   lk_en                             :  2 ;  /*      [3:2] */
      unsigned int   mode                              :  2 ;  /*      [1:0] */
   } cfg ;

#define CACHE_CFG_MODE_TWO_WAY                                 2
#define CACHE_CFG_MODE_ONE_WAY                                 0

#define CACHE_CFG_LK_EN_BOTH                                   3
#define CACHE_CFG_LK_EN_BANK1                                  2
#define CACHE_CFG_LK_EN_BANK0                                  1
#define CACHE_CFG_LK_EN_NONE                                   0

#define CACHE_CFG_WR_SEL_ALWAYS                                3
#define CACHE_CFG_WR_SEL_FETCH                                 2
#define CACHE_CFG_WR_SEL_HIT                                   1
#define CACHE_CFG_WR_SEL_MISS                                  0

#define CACHE_CFG_MODE_MASK                                              0x0003
#define CACHE_CFG_LK_EN_MASK                                             0x000C
#define CACHE_CFG_LK_BIT_EN_MASK                                         0x0010
#define CACHE_CFG_WR_SEL_MASK                                            0x00C0
#define CACHE_CFG_WAITS_MASK                                             0x0700

   struct
   {
      unsigned int   __fill0                           :  9 ;  /*     [15:7] */
      unsigned int   rst_hit_sts                       :  1 ;  /*        [6] */
      unsigned int   hit_sts                           :  4 ;  /*      [5:2] */
      unsigned int   bank_en                           :  2 ;  /*      [1:0] */
   } ctrl ;

#define CACHE_CTRL_BANK_EN_BOTH                                3
#define CACHE_CTRL_BANK_EN_BANK0                               1
#define CACHE_CTRL_BANK_EN_NONE                                0

#define CACHE_CTRL_BANK_EN_MASK                                          0x0003
#define CACHE_CTRL_HIT_STS_MASK                                          0x003C
#define CACHE_CTRL_RST_HIT_STS_MASK                                      0x0040

} ;



struct __mmu_fd_tag
{
   struct
   {
      unsigned int   __fill0                           :  3 ;  /*    [15:13] */
      unsigned int   ext_cs1_data1                     :  1 ;  /*       [12] */
      unsigned int   ext_cs1_data0                     :  1 ;  /*       [11] */
      unsigned int   ext_cs0_data1                     :  1 ;  /*       [10] */
      unsigned int   ext_cs0_data0                     :  1 ;  /*        [9] */
      unsigned int   cache1_data                       :  1 ;  /*        [8] */
      unsigned int   cache0_data                       :  1 ;  /*        [7] */
      unsigned int   ram_data1                         :  1 ;  /*        [6] */
      unsigned int   __fill1                           :  1 ;  /*        [5] */
      unsigned int   flash_data                        :  1 ;  /*        [4] */
      unsigned int   ext_cs1_code                      :  1 ;  /*        [3] */
      unsigned int   ext_cs0_code                      :  1 ;  /*        [2] */
      unsigned int   ram_code                          :  1 ;  /*        [1] */
      unsigned int   __fill2                           :  1 ;  /*        [0] */
   } translate_en ;

#define MMU_TRANSLATE_EN_RAM_CODE_MASK                                   0x0002
#define MMU_TRANSLATE_EN_EXT_CS0_CODE_MASK                               0x0004
#define MMU_TRANSLATE_EN_EXT_CS1_CODE_MASK                               0x0008
#define MMU_TRANSLATE_EN_FLASH_DATA_MASK                                 0x0010
#define MMU_TRANSLATE_EN_RAM_DATA1_MASK                                  0x0040
#define MMU_TRANSLATE_EN_CACHE0_DATA_MASK                                0x0080
#define MMU_TRANSLATE_EN_CACHE1_DATA_MASK                                0x0100
#define MMU_TRANSLATE_EN_EXT_CS0_DATA0_MASK                              0x0200
#define MMU_TRANSLATE_EN_EXT_CS0_DATA1_MASK                              0x0400
#define MMU_TRANSLATE_EN_EXT_CS1_DATA0_MASK                              0x0800
#define MMU_TRANSLATE_EN_EXT_CS1_DATA1_MASK                              0x1000

   struct
   {
      unsigned int   flash_code_log                    : 16 ;  /*     [15:0] */
   } flash_code_log ;

#define MMU_FLASH_CODE_LOG_FLASH_CODE_LOG_MASK                           0xFFFF

   struct
   {
      unsigned int   __fill0                           :  8 ;  /*     [15:8] */
      unsigned int   flash_code_phy                    :  8 ;  /*      [7:0] */
   } flash_code_phy ;

#define MMU_FLASH_CODE_PHY_FLASH_CODE_PHY_MASK                           0x00FF

   struct
   {
      unsigned int   __fill0                           :  8 ;  /*     [15:8] */
      unsigned int   flash_code_size                   :  8 ;  /*      [7:0] */
   } flash_code_size ;

#define MMU_FLASH_CODE_SIZE_FLASH_CODE_SIZE_MASK                         0x00FF

   struct
   {
      unsigned int   ram_code_log                      : 16 ;  /*     [15:0] */
   } ram_code_log ;

#define MMU_RAM_CODE_LOG_RAM_CODE_LOG_MASK                               0xFFFF

   struct
   {
      unsigned int   __fill0                           :  8 ;  /*     [15:8] */
      unsigned int   ram_code_phy                      :  8 ;  /*      [7:0] */
   } ram_code_phy ;

#define MMU_RAM_CODE_PHY_RAM_CODE_PHY_MASK                               0x00FF

   struct
   {
      unsigned int   __fill0                           :  8 ;  /*     [15:8] */
      unsigned int   ram_code_size                     :  8 ;  /*      [7:0] */
   } ram_code_size ;

#define MMU_RAM_CODE_SIZE_RAM_CODE_SIZE_MASK                             0x00FF

   struct
   {
      unsigned int   ext_cs0_code_log                  : 16 ;  /*     [15:0] */
   } ext_cs0_code_log ;

#define MMU_EXT_CS0_CODE_LOG_EXT_CS0_CODE_LOG_MASK                       0xFFFF

   struct
   {
      unsigned int   ext_cs0_code_phy                  : 16 ;  /*     [15:0] */
   } ext_cs0_code_phy ;

#define MMU_EXT_CS0_CODE_PHY_EXT_CS0_CODE_PHY_MASK                       0xFFFF

   struct
   {
      unsigned int   ext_cs0_code_size                 : 16 ;  /*     [15:0] */
   } ext_cs0_code_size ;

#define MMU_EXT_CS0_CODE_SIZE_EXT_CS0_CODE_SIZE_MASK                     0xFFFF

   struct
   {
      unsigned int   ext_cs1_code_log                  : 16 ;  /*     [15:0] */
   } ext_cs1_code_log ;

#define MMU_EXT_CS1_CODE_LOG_EXT_CS1_CODE_LOG_MASK                       0xFFFF

   struct
   {
      unsigned int   ext_cs1_code_phy                  : 16 ;  /*     [15:0] */
   } ext_cs1_code_phy ;

#define MMU_EXT_CS1_CODE_PHY_EXT_CS1_CODE_PHY_MASK                       0xFFFF

   struct
   {
      unsigned int   ect_cs1_code_size                 : 16 ;  /*     [15:0] */
   } ext_cs1_code_size ;

#define MMU_EXT_CS1_CODE_SIZE_ECT_CS1_CODE_SIZE_MASK                     0xFFFF

   struct
   {
      unsigned int   flash_data_log                    : 16 ;  /*     [15:0] */
   } flash_data_log ;

#define MMU_FLASH_DATA_LOG_FLASH_DATA_LOG_MASK                           0xFFFF

   struct
   {
      unsigned int   __fill0                           :  8 ;  /*     [15:8] */
      unsigned int   flash_data_phy                    :  8 ;  /*      [7:0] */
   } flash_data_phy ;

#define MMU_FLASH_DATA_PHY_FLASH_DATA_PHY_MASK                           0x00FF

   struct
   {
      unsigned int   __fill0                           :  8 ;  /*     [15:8] */
      unsigned int   flash_data_size                   :  8 ;  /*      [7:0] */
   } flash_data_size ;

#define MMU_FLASH_DATA_SIZE_FLASH_DATA_SIZE_MASK                         0x00FF

   struct
   {
      unsigned int   ram_data0_log                     : 16 ;  /*     [15:0] */
   } ram_data0_log ;

#define MMU_RAM_DATA0_LOG_RAM_DATA0_LOG_MASK                             0xFFFF

   struct
   {
      unsigned int   __fill0                           :  8 ;  /*     [15:8] */
      unsigned int   ram_data0_phy                     :  8 ;  /*      [7:0] */
   } ram_data0_phy ;

#define MMU_RAM_DATA0_PHY_RAM_DATA0_PHY_MASK                             0x00FF

   struct
   {
      unsigned int   __fill0                           :  8 ;  /*     [15:8] */
      unsigned int   ram_data0_size                    :  8 ;  /*      [7:0] */
   } ram_data0_size ;

#define MMU_RAM_DATA0_SIZE_RAM_DATA0_SIZE_MASK                           0x00FF

   struct
   {
      unsigned int   ram_data1_log                     : 16 ;  /*     [15:0] */
   } ram_data1_log ;

#define MMU_RAM_DATA1_LOG_RAM_DATA1_LOG_MASK                             0xFFFF

   struct
   {
      unsigned int   __fill0                           :  8 ;  /*     [15:8] */
      unsigned int   ram_data1_phy                     :  8 ;  /*      [7:0] */
   } ram_data1_phy ;

#define MMU_RAM_DATA1_PHY_RAM_DATA1_PHY_MASK                             0x00FF

   struct
   {
      unsigned int   __fill0                           :  8 ;  /*     [15:8] */
      unsigned int   ram_data1_size                    :  8 ;  /*      [7:0] */
   } ram_data1_size ;

#define MMU_RAM_DATA1_SIZE_RAM_DATA1_SIZE_MASK                           0x00FF

   struct
   {
      unsigned int   cache0_data_log                   : 16 ;  /*     [15:0] */
   } cache0_data_log ;

#define MMU_CACHE0_DATA_LOG_CACHE0_DATA_LOG_MASK                         0xFFFF

   struct
   {
      unsigned int   cache1_data_log                   : 16 ;  /*     [15:0] */
   } cache1_data_log ;

#define MMU_CACHE1_DATA_LOG_CACHE1_DATA_LOG_MASK                         0xFFFF

   struct
   {
      unsigned int   ext_cs0_data0_log                 : 16 ;  /*     [15:0] */
   } ext_cs0_data0_log ;

#define MMU_EXT_CS0_DATA0_LOG_EXT_CS0_DATA0_LOG_MASK                     0xFFFF

   struct
   {
      unsigned int   ext_cs0_data0_phy                 : 16 ;  /*     [15:0] */
   } ext_cs0_data0_phy ;

#define MMU_EXT_CS0_DATA0_PHY_EXT_CS0_DATA0_PHY_MASK                     0xFFFF

   struct
   {
      unsigned int   __fill0                           :  8 ;  /*     [15:8] */
      unsigned int   ext_cs0_data0_size                :  8 ;  /*      [7:0] */
   } ext_cs0_data0_size ;

#define MMU_EXT_CS0_DATA0_SIZE_EXT_CS0_DATA0_SIZE_MASK                   0x00FF

   struct
   {
      unsigned int   ext_cs0_data1_log                 : 16 ;  /*     [15:0] */
   } ext_cs0_data1_log ;

#define MMU_EXT_CS0_DATA1_LOG_EXT_CS0_DATA1_LOG_MASK                     0xFFFF

   struct
   {
      unsigned int   ext_cs0_data1_phy                 : 16 ;  /*     [15:0] */
   } ext_cs0_data1_phy ;

#define MMU_EXT_CS0_DATA1_PHY_EXT_CS0_DATA1_PHY_MASK                     0xFFFF

   struct
   {
      unsigned int   __fill0                           :  8 ;  /*     [15:8] */
      unsigned int   ext_cs0_data1_size                :  8 ;  /*      [7:0] */
   } ext_cs0_data1_size ;

#define MMU_EXT_CS0_DATA1_SIZE_EXT_CS0_DATA1_SIZE_MASK                   0x00FF

   struct
   {
      unsigned int   ext_cs1_data0_log                 : 16 ;  /*     [15:0] */
   } ext_cs1_data0_log ;

#define MMU_EXT_CS1_DATA0_LOG_EXT_CS1_DATA0_LOG_MASK                     0xFFFF

   struct
   {
      unsigned int   ext_cs1_data0_phy                 : 16 ;  /*     [15:0] */
   } ext_cs1_data0_phy ;

#define MMU_EXT_CS1_DATA0_PHY_EXT_CS1_DATA0_PHY_MASK                     0xFFFF

   struct
   {
      unsigned int   __fill0                           :  8 ;  /*     [15:8] */
      unsigned int   ext_cs1_data0_size                :  8 ;  /*      [7:0] */
   } ext_cs1_data0_size ;

#define MMU_EXT_CS1_DATA0_SIZE_EXT_CS1_DATA0_SIZE_MASK                   0x00FF

   struct
   {
      unsigned int   ext_cs1_data1_log                 : 16 ;  /*     [15:0] */
   } ext_cs1_data1_log ;

#define MMU_EXT_CS1_DATA1_LOG_EXT_CS1_DATA1_LOG_MASK                     0xFFFF

   struct
   {
      unsigned int   ext_cs1_data1_phy                 : 16 ;  /*     [15:0] */
   } ext_cs1_data1_phy ;

#define MMU_EXT_CS1_DATA1_PHY_EXT_CS1_DATA1_PHY_MASK                     0xFFFF

   struct
   {
      unsigned int   __fill0                           :  8 ;  /*     [15:8] */
      unsigned int   ext_cs1_data1_size                :  8 ;  /*      [7:0] */
   } ext_cs1_data1_size ;

#define MMU_EXT_CS1_DATA1_SIZE_EXT_CS1_DATA1_SIZE_MASK                   0x00FF

   struct
   {
      unsigned int   __fill0                           : 11 ;  /*     [15:5] */
      unsigned int   cache_dis                         :  1 ;  /*        [4] */
      unsigned int   wait_states                       :  3 ;  /*      [3:1] */
      unsigned int   early_req_en                      :  1 ;  /*        [0] */
   } flash_ctrl ;

#define MMU_FLASH_CTRL_EARLY_REQ_EN_MASK                                 0x0001
#define MMU_FLASH_CTRL_WAIT_STATES_MASK                                  0x000E
#define MMU_FLASH_CTRL_CACHE_DIS_MASK                                    0x0010

   struct
   {
      unsigned int   __fill0                           : 10 ;  /*     [15:6] */
      unsigned int   if_clk_per                        :  2 ;  /*      [5:4] */
      unsigned int   dma2_little_endian                :  1 ;  /*        [3] */
      unsigned int   dma1_little_endian                :  1 ;  /*        [2] */
      unsigned int   cache_dis                         :  1 ;  /*        [1] */
      unsigned int   read_wait_state                   :  1 ;  /*        [0] */
   } ram_ctrl ;

#define MMU_RAM_CTRL_IF_CLK_PER_SIXTEEN                        3
#define MMU_RAM_CTRL_IF_CLK_PER_SIX                            2
#define MMU_RAM_CTRL_IF_CLK_PER_FOUR                           1
#define MMU_RAM_CTRL_IF_CLK_PER_TWO                            0

#define MMU_RAM_CTRL_READ_WAIT_STATE_MASK                                0x0001
#define MMU_RAM_CTRL_CACHE_DIS_MASK                                      0x0002
#define MMU_RAM_CTRL_DMA1_LITTLE_ENDIAN_MASK                             0x0004
#define MMU_RAM_CTRL_DMA2_LITTLE_ENDIAN_MASK                             0x0008
#define MMU_RAM_CTRL_IF_CLK_PER_MASK                                     0x0030

   struct
   {
      unsigned int   __fill0                           : 12 ;  /*     [15:4] */
      unsigned int   data_int_clr                      :  1 ;  /*        [3] */
      unsigned int   data_sts                          :  1 ;  /*        [2] */
      unsigned int   code_int_clr                      :  1 ;  /*        [1] */
      unsigned int   code_sts                          :  1 ;  /*        [0] */
   } adr_err ;

#define MMU_ADR_ERR_CODE_STS_MASK                                        0x0001
#define MMU_ADR_ERR_CODE_INT_CLR_MASK                                    0x0002
#define MMU_ADR_ERR_DATA_STS_MASK                                        0x0004
#define MMU_ADR_ERR_DATA_INT_CLR_MASK                                    0x0008

} ;



struct __ssm_fd_tag
{
   struct
   {
      unsigned int   intact                            :  1 ;  /*       [15] */
      unsigned int   emi                               :  1 ;  /*       [14] */
      unsigned int   ltmr                              :  1 ;  /*       [13] */
      unsigned int   tmr                               :  1 ;  /*       [12] */
      unsigned int   ex                                :  1 ;  /*       [11] */
      unsigned int   cap                               :  1 ;  /*       [10] */
      unsigned int   pwm2                              :  1 ;  /*        [9] */
      unsigned int   pwm1                              :  1 ;  /*        [8] */
      unsigned int   cnt2                              :  1 ;  /*        [7] */
      unsigned int   cnt1                              :  1 ;  /*        [6] */
      unsigned int   dusart                            :  1 ;  /*        [5] */
      unsigned int   duart                             :  1 ;  /*        [4] */
      unsigned int   high_ref_div_chn                  :  1 ;  /*        [3] */
      unsigned int   high_pll_div_chn                  :  1 ;  /*        [2] */
      unsigned int   low_ref_div_chn                   :  1 ;  /*        [1] */
      unsigned int   low_pll_div_chn                   :  1 ;  /*        [0] */
   } rst_set ;

#define SSM_RST_SET_LOW_PLL_DIV_CHN_MASK                                 0x0001
#define SSM_RST_SET_LOW_REF_DIV_CHN_MASK                                 0x0002
#define SSM_RST_SET_HIGH_PLL_DIV_CHN_MASK                                0x0004
#define SSM_RST_SET_HIGH_REF_DIV_CHN_MASK                                0x0008
#define SSM_RST_SET_DUART_MASK                                           0x0010
#define SSM_RST_SET_DUSART_MASK                                          0x0020
#define SSM_RST_SET_CNT1_MASK                                            0x0040
#define SSM_RST_SET_CNT2_MASK                                            0x0080
#define SSM_RST_SET_PWM1_MASK                                            0x0100
#define SSM_RST_SET_PWM2_MASK                                            0x0200
#define SSM_RST_SET_CAP_MASK                                             0x0400
#define SSM_RST_SET_EX_MASK                                              0x0800
#define SSM_RST_SET_TMR_MASK                                             0x1000
#define SSM_RST_SET_LTMR_MASK                                            0x2000
#define SSM_RST_SET_EMI_MASK                                             0x4000
#define SSM_RST_SET_INTACT_MASK                                          0x8000

   struct
   {
      unsigned int   intact                            :  1 ;  /*       [15] */
      unsigned int   emi                               :  1 ;  /*       [14] */
      unsigned int   ltmr                              :  1 ;  /*       [13] */
      unsigned int   tmr                               :  1 ;  /*       [12] */
      unsigned int   ex                                :  1 ;  /*       [11] */
      unsigned int   cap                               :  1 ;  /*       [10] */
      unsigned int   pwm2                              :  1 ;  /*        [9] */
      unsigned int   pwm1                              :  1 ;  /*        [8] */
      unsigned int   cnt2                              :  1 ;  /*        [7] */
      unsigned int   cnt1                              :  1 ;  /*        [6] */
      unsigned int   dusart                            :  1 ;  /*        [5] */
      unsigned int   duart                             :  1 ;  /*        [4] */
      unsigned int   high_ref_div_chn                  :  1 ;  /*        [3] */
      unsigned int   high_pll_div_chn                  :  1 ;  /*        [2] */
      unsigned int   low_ref_div_chn                   :  1 ;  /*        [1] */
      unsigned int   low_pll_div_chn                   :  1 ;  /*        [0] */
   } rst_clr ;

#define SSM_RST_CLR_LOW_PLL_DIV_CHN_MASK                                 0x0001
#define SSM_RST_CLR_LOW_REF_DIV_CHN_MASK                                 0x0002
#define SSM_RST_CLR_HIGH_PLL_DIV_CHN_MASK                                0x0004
#define SSM_RST_CLR_HIGH_REF_DIV_CHN_MASK                                0x0008
#define SSM_RST_CLR_DUART_MASK                                           0x0010
#define SSM_RST_CLR_DUSART_MASK                                          0x0020
#define SSM_RST_CLR_CNT1_MASK                                            0x0040
#define SSM_RST_CLR_CNT2_MASK                                            0x0080
#define SSM_RST_CLR_PWM1_MASK                                            0x0100
#define SSM_RST_CLR_PWM2_MASK                                            0x0200
#define SSM_RST_CLR_CAP_MASK                                             0x0400
#define SSM_RST_CLR_EX_MASK                                              0x0800
#define SSM_RST_CLR_TMR_MASK                                             0x1000
#define SSM_RST_CLR_LTMR_MASK                                            0x2000
#define SSM_RST_CLR_EMI_MASK                                             0x4000
#define SSM_RST_CLR_INTACT_MASK                                          0x8000

   struct
   {
      unsigned int   high_osc                          :  1 ;  /*       [15] */
      unsigned int   emi                               :  1 ;  /*       [14] */
      unsigned int   ltmr                              :  1 ;  /*       [13] */
      unsigned int   tmr                               :  1 ;  /*       [12] */
      unsigned int   ex                                :  1 ;  /*       [11] */
      unsigned int   cap                               :  1 ;  /*       [10] */
      unsigned int   pwm2                              :  1 ;  /*        [9] */
      unsigned int   pwm1                              :  1 ;  /*        [8] */
      unsigned int   cnt2                              :  1 ;  /*        [7] */
      unsigned int   cnt1                              :  1 ;  /*        [6] */
      unsigned int   dusart                            :  1 ;  /*        [5] */
      unsigned int   uartb                             :  1 ;  /*        [4] */
      unsigned int   uarta                             :  1 ;  /*        [3] */
      unsigned int   intact                            :  1 ;  /*        [2] */
      unsigned int   low_pll                           :  1 ;  /*        [1] */
      unsigned int   high_pll                          :  1 ;  /*        [0] */
   } clk_en ;

#define SSM_CLK_EN_HIGH_PLL_MASK                                         0x0001
#define SSM_CLK_EN_LOW_PLL_MASK                                          0x0002
#define SSM_CLK_EN_INTACT_MASK                                           0x0004
#define SSM_CLK_EN_UARTA_MASK                                            0x0008
#define SSM_CLK_EN_UARTB_MASK                                            0x0010
#define SSM_CLK_EN_DUSART_MASK                                           0x0020
#define SSM_CLK_EN_CNT1_MASK                                             0x0040
#define SSM_CLK_EN_CNT2_MASK                                             0x0080
#define SSM_CLK_EN_PWM1_MASK                                             0x0100
#define SSM_CLK_EN_PWM2_MASK                                             0x0200
#define SSM_CLK_EN_CAP_MASK                                              0x0400
#define SSM_CLK_EN_EX_MASK                                               0x0800
#define SSM_CLK_EN_TMR_MASK                                              0x1000
#define SSM_CLK_EN_LTMR_MASK                                             0x2000
#define SSM_CLK_EN_EMI_MASK                                              0x4000
#define SSM_CLK_EN_HIGH_OSC_MASK                                         0x8000

   struct
   {
      unsigned int   high_osc                          :  1 ;  /*       [15] */
      unsigned int   emi                               :  1 ;  /*       [14] */
      unsigned int   ltmr                              :  1 ;  /*       [13] */
      unsigned int   tmr                               :  1 ;  /*       [12] */
      unsigned int   ex                                :  1 ;  /*       [11] */
      unsigned int   cap                               :  1 ;  /*       [10] */
      unsigned int   pwm2                              :  1 ;  /*        [9] */
      unsigned int   pwm1                              :  1 ;  /*        [8] */
      unsigned int   cnt2                              :  1 ;  /*        [7] */
      unsigned int   cnt1                              :  1 ;  /*        [6] */
      unsigned int   dusart                            :  1 ;  /*        [5] */
      unsigned int   uartb                             :  1 ;  /*        [4] */
      unsigned int   uarta                             :  1 ;  /*        [3] */
      unsigned int   intact                            :  1 ;  /*        [2] */
      unsigned int   low_pll                           :  1 ;  /*        [1] */
      unsigned int   high_pll                          :  1 ;  /*        [0] */
   } clk_dis ;

#define SSM_CLK_DIS_HIGH_PLL_MASK                                        0x0001
#define SSM_CLK_DIS_LOW_PLL_MASK                                         0x0002
#define SSM_CLK_DIS_INTACT_MASK                                          0x0004
#define SSM_CLK_DIS_UARTA_MASK                                           0x0008
#define SSM_CLK_DIS_UARTB_MASK                                           0x0010
#define SSM_CLK_DIS_DUSART_MASK                                          0x0020
#define SSM_CLK_DIS_CNT1_MASK                                            0x0040
#define SSM_CLK_DIS_CNT2_MASK                                            0x0080
#define SSM_CLK_DIS_PWM1_MASK                                            0x0100
#define SSM_CLK_DIS_PWM2_MASK                                            0x0200
#define SSM_CLK_DIS_CAP_MASK                                             0x0400
#define SSM_CLK_DIS_EX_MASK                                              0x0800
#define SSM_CLK_DIS_TMR_MASK                                             0x1000
#define SSM_CLK_DIS_LTMR_MASK                                            0x2000
#define SSM_CLK_DIS_EMI_MASK                                             0x4000
#define SSM_CLK_DIS_HIGH_OSC_MASK                                        0x8000

   struct
   {
      unsigned int   __fill0                           :  2 ;  /*    [15:14] */
      unsigned int   ltmr                              :  1 ;  /*       [13] */
      unsigned int   tmr                               :  1 ;  /*       [12] */
      unsigned int   ex                                :  1 ;  /*       [11] */
      unsigned int   cap                               :  1 ;  /*       [10] */
      unsigned int   pwm2                              :  1 ;  /*        [9] */
      unsigned int   pwm1                              :  1 ;  /*        [8] */
      unsigned int   cnt2                              :  1 ;  /*        [7] */
      unsigned int   cnt1                              :  1 ;  /*        [6] */
      unsigned int   dusart                            :  1 ;  /*        [5] */
      unsigned int   uartb                             :  1 ;  /*        [4] */
      unsigned int   uarta                             :  1 ;  /*        [3] */
      unsigned int   intact                            :  1 ;  /*        [2] */
      unsigned int   __fill1                           :  2 ;  /*      [1:0] */
   } clk_deact ;

#define SSM_CLK_DEACT_INTACT_MASK                                        0x0004
#define SSM_CLK_DEACT_UARTA_MASK                                         0x0008
#define SSM_CLK_DEACT_UARTB_MASK                                         0x0010
#define SSM_CLK_DEACT_DUSART_MASK                                        0x0020
#define SSM_CLK_DEACT_CNT1_MASK                                          0x0040
#define SSM_CLK_DEACT_CNT2_MASK                                          0x0080
#define SSM_CLK_DEACT_PWM1_MASK                                          0x0100
#define SSM_CLK_DEACT_PWM2_MASK                                          0x0200
#define SSM_CLK_DEACT_CAP_MASK                                           0x0400
#define SSM_CLK_DEACT_EX_MASK                                            0x0800
#define SSM_CLK_DEACT_TMR_MASK                                           0x1000
#define SSM_CLK_DEACT_LTMR_MASK                                          0x2000

   struct
   {
      unsigned int   timeout                           :  1 ;  /*       [15] */
      unsigned int   __fill0                           :  1 ;  /*       [14] */
      unsigned int   ltmr                              :  1 ;  /*       [13] */
      unsigned int   tmr                               :  1 ;  /*       [12] */
      unsigned int   ex                                :  1 ;  /*       [11] */
      unsigned int   cap                               :  1 ;  /*       [10] */
      unsigned int   pwm2                              :  1 ;  /*        [9] */
      unsigned int   pwm1                              :  1 ;  /*        [8] */
      unsigned int   cnt2                              :  1 ;  /*        [7] */
      unsigned int   cnt1                              :  1 ;  /*        [6] */
      unsigned int   dusart                            :  1 ;  /*        [5] */
      unsigned int   uartb                             :  1 ;  /*        [4] */
      unsigned int   uarta                             :  1 ;  /*        [3] */
      unsigned int   intact                            :  1 ;  /*        [2] */
      unsigned int   __fill1                           :  2 ;  /*      [1:0] */
   } clk_sleep_dis ;

#define SSM_CLK_SLEEP_DIS_INTACT_MASK                                    0x0004
#define SSM_CLK_SLEEP_DIS_UARTA_MASK                                     0x0008
#define SSM_CLK_SLEEP_DIS_UARTB_MASK                                     0x0010
#define SSM_CLK_SLEEP_DIS_DUSART_MASK                                    0x0020
#define SSM_CLK_SLEEP_DIS_CNT1_MASK                                      0x0040
#define SSM_CLK_SLEEP_DIS_CNT2_MASK                                      0x0080
#define SSM_CLK_SLEEP_DIS_PWM1_MASK                                      0x0100
#define SSM_CLK_SLEEP_DIS_PWM2_MASK                                      0x0200
#define SSM_CLK_SLEEP_DIS_CAP_MASK                                       0x0400
#define SSM_CLK_SLEEP_DIS_EX_MASK                                        0x0800
#define SSM_CLK_SLEEP_DIS_TMR_MASK                                       0x1000
#define SSM_CLK_SLEEP_DIS_LTMR_MASK                                      0x2000
#define SSM_CLK_SLEEP_DIS_TIMEOUT_MASK                                   0x8000

   struct
   {
      unsigned int   __fill0                           :  2 ;  /*    [15:14] */
      unsigned int   ltmr                              :  1 ;  /*       [13] */
      unsigned int   tmr                               :  1 ;  /*       [12] */
      unsigned int   ex                                :  1 ;  /*       [11] */
      unsigned int   cap                               :  1 ;  /*       [10] */
      unsigned int   pwm2                              :  1 ;  /*        [9] */
      unsigned int   pwm1                              :  1 ;  /*        [8] */
      unsigned int   cnt2                              :  1 ;  /*        [7] */
      unsigned int   cnt1                              :  1 ;  /*        [6] */
      unsigned int   dusart                            :  1 ;  /*        [5] */
      unsigned int   uartb                             :  1 ;  /*        [4] */
      unsigned int   uarta                             :  1 ;  /*        [3] */
      unsigned int   intact                            :  1 ;  /*        [2] */
      unsigned int   __fill1                           :  2 ;  /*      [1:0] */
   } clk_wake_en ;

#define SSM_CLK_WAKE_EN_INTACT_MASK                                      0x0004
#define SSM_CLK_WAKE_EN_UARTA_MASK                                       0x0008
#define SSM_CLK_WAKE_EN_UARTB_MASK                                       0x0010
#define SSM_CLK_WAKE_EN_DUSART_MASK                                      0x0020
#define SSM_CLK_WAKE_EN_CNT1_MASK                                        0x0040
#define SSM_CLK_WAKE_EN_CNT2_MASK                                        0x0080
#define SSM_CLK_WAKE_EN_PWM1_MASK                                        0x0100
#define SSM_CLK_WAKE_EN_PWM2_MASK                                        0x0200
#define SSM_CLK_WAKE_EN_CAP_MASK                                         0x0400
#define SSM_CLK_WAKE_EN_EX_MASK                                          0x0800
#define SSM_CLK_WAKE_EN_TMR_MASK                                         0x1000
#define SSM_CLK_WAKE_EN_LTMR_MASK                                        0x2000

   unsigned int __fill0 ;                                      /*       FF71 */

   struct
   {
      unsigned int   __fill0                           :  4 ;  /*    [15:12] */
      unsigned int   sts                               :  4 ;  /*     [11:8] */
      unsigned int   __fill1                           :  2 ;  /*      [7:6] */
      unsigned int   cpu_clk_div                       :  3 ;  /*      [5:3] */
      unsigned int   prescalar                         :  3 ;  /*      [2:0] */
   } cpu ;

#define SSM_CPU_PRESCALAR_DIV2                                 7
#define SSM_CPU_PRESCALAR_DIV4                                 6
#define SSM_CPU_PRESCALAR_DIV6                                 5
#define SSM_CPU_PRESCALAR_DIV8                                 4
#define SSM_CPU_PRESCALAR_DIV10                                3
#define SSM_CPU_PRESCALAR_DIV12                                2
#define SSM_CPU_PRESCALAR_DIV14                                1
#define SSM_CPU_PRESCALAR_DIV16                                0

#define SSM_CPU_CPU_CLK_DIV_DIV1                               7
#define SSM_CPU_CPU_CLK_DIV_DIV2                               6
#define SSM_CPU_CPU_CLK_DIV_DIV3                               5
#define SSM_CPU_CPU_CLK_DIV_DIV4                               4
#define SSM_CPU_CPU_CLK_DIV_DIV5                               3
#define SSM_CPU_CPU_CLK_DIV_DIV6                               2
#define SSM_CPU_CPU_CLK_DIV_DIV7                               1
#define SSM_CPU_CPU_CLK_DIV_DIV8                               0

#define SSM_CPU_STS_HIGH_REF_CLK                               8
#define SSM_CPU_STS_LOW_REF_CLK                                4
#define SSM_CPU_STS_HIGH_PLL_CLK                               2
#define SSM_CPU_STS_LOW_PLL_CLK                                1

#define SSM_CPU_PRESCALAR_MASK                                           0x0007
#define SSM_CPU_CPU_CLK_DIV_MASK                                         0x0038
#define SSM_CPU_STS_MASK                                                 0x0F00

   struct
   {
      unsigned int   high_osc_dis                      :  1 ;  /*       [15] */
      unsigned int   emi                               :  1 ;  /*       [14] */
      unsigned int   ltmr                              :  1 ;  /*       [13] */
      unsigned int   tmr                               :  1 ;  /*       [12] */
      unsigned int   ex                                :  1 ;  /*       [11] */
      unsigned int   cap                               :  1 ;  /*       [10] */
      unsigned int   pwm2                              :  1 ;  /*        [9] */
      unsigned int   pwm1                              :  1 ;  /*        [8] */
      unsigned int   cnt2                              :  1 ;  /*        [7] */
      unsigned int   cnt1                              :  1 ;  /*        [6] */
      unsigned int   dusart                            :  1 ;  /*        [5] */
      unsigned int   uartb                             :  1 ;  /*        [4] */
      unsigned int   uarta                             :  1 ;  /*        [3] */
      unsigned int   intact                            :  1 ;  /*        [2] */
      unsigned int   low_pll                           :  1 ;  /*        [1] */
      unsigned int   high_pll                          :  1 ;  /*        [0] */
   } sts ;

#define SSM_STS_HIGH_PLL_MASK                                            0x0001
#define SSM_STS_LOW_PLL_MASK                                             0x0002
#define SSM_STS_INTACT_MASK                                              0x0004
#define SSM_STS_UARTA_MASK                                               0x0008
#define SSM_STS_UARTB_MASK                                               0x0010
#define SSM_STS_DUSART_MASK                                              0x0020
#define SSM_STS_CNT1_MASK                                                0x0040
#define SSM_STS_CNT2_MASK                                                0x0080
#define SSM_STS_PWM1_MASK                                                0x0100
#define SSM_STS_PWM2_MASK                                                0x0200
#define SSM_STS_CAP_MASK                                                 0x0400
#define SSM_STS_EX_MASK                                                  0x0800
#define SSM_STS_TMR_MASK                                                 0x1000
#define SSM_STS_LTMR_MASK                                                0x2000
#define SSM_STS_EMI_MASK                                                 0x4000
#define SSM_STS_HIGH_OSC_DIS_MASK                                        0x8000

   struct
   {
      unsigned int   __fill0                           :  5 ;  /*    [15:11] */
      unsigned int   adc_en                            :  1 ;  /*       [10] */
      unsigned int   pll_stepup                        :  1 ;  /*        [9] */
      unsigned int   __fill1                           :  3 ;  /*      [8:6] */
      unsigned int   clk_sel                           :  2 ;  /*      [5:4] */
      unsigned int   doze_dis                          :  1 ;  /*        [3] */
      unsigned int   wakeon_ip_en                      :  1 ;  /*        [2] */
      unsigned int   wakeon_if_dis                     :  1 ;  /*        [1] */
      unsigned int   clk_en                            :  1 ;  /*        [0] */
   } cfg ;

#define SSM_CFG_CLK_SEL_LOW_PLL_CLK                            3
#define SSM_CFG_CLK_SEL_HIGH_PLL_CLK                           2
#define SSM_CFG_CLK_SEL_LOW_REF_CLK                            1
#define SSM_CFG_CLK_SEL_HIGH_REF_CLK                           0

#define SSM_CFG_CLK_EN_MASK                                              0x0001
#define SSM_CFG_WAKEON_IF_DIS_MASK                                       0x0002
#define SSM_CFG_WAKEON_IP_EN_MASK                                        0x0004
#define SSM_CFG_DOZE_DIS_MASK                                            0x0008
#define SSM_CFG_CLK_SEL_MASK                                             0x0030
#define SSM_CFG_PLL_STEPUP_MASK                                          0x0200
#define SSM_CFG_ADC_EN_MASK                                              0x0400

   struct
   {
      unsigned int   __fill0                           :  1 ;  /*       [15] */
      unsigned int   low_clk_timeout                   :  1 ;  /*       [14] */
      unsigned int   low_clk_intact                    :  1 ;  /*       [13] */
      unsigned int   low_clk_timer                     :  1 ;  /*       [12] */
      unsigned int   low_clk_dusart                    :  1 ;  /*       [11] */
      unsigned int   low_clk_duart                     :  1 ;  /*       [10] */
      unsigned int   intact                            :  1 ;  /*        [9] */
      unsigned int   tmr                               :  1 ;  /*        [8] */
      unsigned int   ex                                :  1 ;  /*        [7] */
      unsigned int   cap                               :  1 ;  /*        [6] */
      unsigned int   pwm2                              :  1 ;  /*        [5] */
      unsigned int   pwm1                              :  1 ;  /*        [4] */
      unsigned int   cnt2                              :  1 ;  /*        [3] */
      unsigned int   cnt1                              :  1 ;  /*        [2] */
      unsigned int   dusart                            :  1 ;  /*        [1] */
      unsigned int   duart                             :  1 ;  /*        [0] */
   } div_sel ;

#define SSM_DIV_SEL_DUART_MASK                                           0x0001
#define SSM_DIV_SEL_DUSART_MASK                                          0x0002
#define SSM_DIV_SEL_CNT1_MASK                                            0x0004
#define SSM_DIV_SEL_CNT2_MASK                                            0x0008
#define SSM_DIV_SEL_PWM1_MASK                                            0x0010
#define SSM_DIV_SEL_PWM2_MASK                                            0x0020
#define SSM_DIV_SEL_CAP_MASK                                             0x0040
#define SSM_DIV_SEL_EX_MASK                                              0x0080
#define SSM_DIV_SEL_TMR_MASK                                             0x0100
#define SSM_DIV_SEL_INTACT_MASK                                          0x0200
#define SSM_DIV_SEL_LOW_CLK_DUART_MASK                                   0x0400
#define SSM_DIV_SEL_LOW_CLK_DUSART_MASK                                  0x0800
#define SSM_DIV_SEL_LOW_CLK_TIMER_MASK                                   0x1000
#define SSM_DIV_SEL_LOW_CLK_INTACT_MASK                                  0x2000
#define SSM_DIV_SEL_LOW_CLK_TIMEOUT_MASK                                 0x4000

   struct
   {
      unsigned int   intact                            :  4 ;  /*    [15:12] */
      unsigned int   dusart                            :  4 ;  /*     [11:8] */
      unsigned int   duart                             :  4 ;  /*      [7:4] */
      unsigned int   timeout                           :  4 ;  /*      [3:0] */
   } tap_sel1 ;

#define SSM_TAP_SEL1_TIMEOUT_MASK                                        0x000F
#define SSM_TAP_SEL1_DUART_MASK                                          0x00F0
#define SSM_TAP_SEL1_DUSART_MASK                                         0x0F00
#define SSM_TAP_SEL1_INTACT_MASK                                         0xF000

   struct
   {
      unsigned int   pwm2                              :  4 ;  /*    [15:12] */
      unsigned int   pwm1                              :  4 ;  /*     [11:8] */
      unsigned int   cnt2                              :  4 ;  /*      [7:4] */
      unsigned int   cnt1                              :  4 ;  /*      [3:0] */
   } tap_sel2 ;

#define SSM_TAP_SEL2_CNT1_MASK                                           0x000F
#define SSM_TAP_SEL2_CNT2_MASK                                           0x00F0
#define SSM_TAP_SEL2_PWM1_MASK                                           0x0F00
#define SSM_TAP_SEL2_PWM2_MASK                                           0xF000

   struct
   {
      unsigned int   ltmr                              :  4 ;  /*    [15:12] */
      unsigned int   tmr                               :  4 ;  /*     [11:8] */
      unsigned int   ex                                :  4 ;  /*      [7:4] */
      unsigned int   cap                               :  4 ;  /*      [3:0] */
   } tap_sel3 ;

#define SSM_TAP_SEL3_CAP_MASK                                            0x000F
#define SSM_TAP_SEL3_EX_MASK                                             0x00F0
#define SSM_TAP_SEL3_TMR_MASK                                            0x0F00
#define SSM_TAP_SEL3_LTMR_MASK                                           0xF000

   struct
   {
      unsigned int   __fill0                           :  7 ;  /*     [15:9] */
      unsigned int   adc_high_ref_clk                  :  1 ;  /*        [8] */
      unsigned int   adc_low_pll_clk                   :  1 ;  /*        [7] */
      unsigned int   adc_rst_clr                       :  1 ;  /*        [6] */
      unsigned int   adc_rst_set                       :  1 ;  /*        [5] */
      unsigned int   intact_core_rst                   :  1 ;  /*        [4] */
      unsigned int   if_rst                            :  1 ;  /*        [3] */
      unsigned int   cpu_rst                           :  1 ;  /*        [2] */
      unsigned int   evening                           :  1 ;  /*        [1] */
      unsigned int   morning                           :  1 ;  /*        [0] */
   } ex_ctrl ;

#define SSM_EX_CTRL_MORNING_MASK                                         0x0001
#define SSM_EX_CTRL_EVENING_MASK                                         0x0002
#define SSM_EX_CTRL_CPU_RST_MASK                                         0x0004
#define SSM_EX_CTRL_IF_RST_MASK                                          0x0008
#define SSM_EX_CTRL_INTACT_CORE_RST_MASK                                 0x0010
#define SSM_EX_CTRL_ADC_RST_SET_MASK                                     0x0020
#define SSM_EX_CTRL_ADC_RST_CLR_MASK                                     0x0040
#define SSM_EX_CTRL_ADC_LOW_PLL_CLK_MASK                                 0x0080
#define SSM_EX_CTRL_ADC_HIGH_REF_CLK_MASK                                0x0100

} ;



struct __emi_fd_tag
{
   struct
   {
      unsigned int   __fill0                           :  3 ;  /*    [15:13] */
      unsigned int   refr_dis                          :  1 ;  /*       [12] */
      unsigned int   refr_en                           :  1 ;  /*       [11] */
      unsigned int   adr_err_sts                       :  1 ;  /*       [10] */
      unsigned int   adr_err_clr                       :  1 ;  /*        [9] */
      unsigned int   adr_err_dis                       :  1 ;  /*        [8] */
      unsigned int   adr_err_en                        :  1 ;  /*        [7] */
      unsigned int   bus_dis                           :  1 ;  /*        [6] */
      unsigned int   bus_en                            :  1 ;  /*        [5] */
      unsigned int   bus_bsy                           :  1 ;  /*        [4] */
      unsigned int   sdram_dis                         :  1 ;  /*        [3] */
      unsigned int   sdram_en                          :  1 ;  /*        [2] */
      unsigned int   sdram_bsy                         :  1 ;  /*        [1] */
      unsigned int   cmd_pending                       :  1 ;  /*        [0] */
   } ctrl_sts ;

#define EMI_CTRL_STS_CMD_PENDING_MASK                                    0x0001
#define EMI_CTRL_STS_SDRAM_BSY_MASK                                      0x0002
#define EMI_CTRL_STS_SDRAM_EN_MASK                                       0x0004
#define EMI_CTRL_STS_SDRAM_DIS_MASK                                      0x0008
#define EMI_CTRL_STS_BUS_BSY_MASK                                        0x0010
#define EMI_CTRL_STS_BUS_EN_MASK                                         0x0020
#define EMI_CTRL_STS_BUS_DIS_MASK                                        0x0040
#define EMI_CTRL_STS_ADR_ERR_EN_MASK                                     0x0080
#define EMI_CTRL_STS_ADR_ERR_DIS_MASK                                    0x0100
#define EMI_CTRL_STS_ADR_ERR_CLR_MASK                                    0x0200
#define EMI_CTRL_STS_ADR_ERR_STS_MASK                                    0x0400
#define EMI_CTRL_STS_REFR_EN_MASK                                        0x0800
#define EMI_CTRL_STS_REFR_DIS_MASK                                       0x1000

   struct
   {
      unsigned int   cs1_pol                           :  1 ;  /*       [15] */
      unsigned int   cs0_pol                           :  1 ;  /*       [14] */
      unsigned int   rw_rs_pol                         :  1 ;  /*       [13] */
      unsigned int   ds_ws_pol                         :  1 ;  /*       [12] */
      unsigned int   thah                              :  2 ;  /*    [11:10] */
      unsigned int   tah                               :  2 ;  /*      [9:8] */
      unsigned int   tcs                               :  2 ;  /*      [7:6] */
      unsigned int   ah_en                             :  1 ;  /*        [5] */
      unsigned int   ds_en                             :  1 ;  /*        [4] */
      unsigned int   rw_mode                           :  1 ;  /*        [3] */
      unsigned int   wait_en                           :  1 ;  /*        [2] */
      unsigned int   word                              :  1 ;  /*        [1] */
      unsigned int   cs_en                             :  1 ;  /*        [0] */
   } bus_cfg1 ;

#define EMI_BUS_CFG1_RW_MODE_DS_RW                             1
#define EMI_BUS_CFG1_RW_MODE_WS_RS                             0

#define EMI_BUS_CFG1_CS_EN_MASK                                          0x0001
#define EMI_BUS_CFG1_WORD_MASK                                           0x0002
#define EMI_BUS_CFG1_WAIT_EN_MASK                                        0x0004
#define EMI_BUS_CFG1_RW_MODE_MASK                                        0x0008
#define EMI_BUS_CFG1_DS_EN_MASK                                          0x0010
#define EMI_BUS_CFG1_AH_EN_MASK                                          0x0020
#define EMI_BUS_CFG1_TCS_MASK                                            0x00C0
#define EMI_BUS_CFG1_TAH_MASK                                            0x0300
#define EMI_BUS_CFG1_THAH_MASK                                           0x0C00
#define EMI_BUS_CFG1_DS_WS_POL_MASK                                      0x1000
#define EMI_BUS_CFG1_RW_RS_POL_MASK                                      0x2000
#define EMI_BUS_CFG1_CS0_POL_MASK                                        0x4000
#define EMI_BUS_CFG1_CS1_POL_MASK                                        0x8000

   struct
   {
      unsigned int   __fill0                           :  2 ;  /*    [15:14] */
      unsigned int   cs1_cache_dis                     :  1 ;  /*       [13] */
      unsigned int   cs0_cache_dis                     :  1 ;  /*       [12] */
      unsigned int   tchr                              :  2 ;  /*    [11:10] */
      unsigned int   tdwr                              :  2 ;  /*      [9:8] */
      unsigned int   tdsr                              :  2 ;  /*      [7:6] */
      unsigned int   tchw                              :  2 ;  /*      [5:4] */
      unsigned int   tdww                              :  2 ;  /*      [3:2] */
      unsigned int   tdsw                              :  2 ;  /*      [1:0] */
   } bus_cfg2 ;

#define EMI_BUS_CFG2_TDSW_MASK                                           0x0003
#define EMI_BUS_CFG2_TDWW_MASK                                           0x000C
#define EMI_BUS_CFG2_TCHW_MASK                                           0x0030
#define EMI_BUS_CFG2_TDSR_MASK                                           0x00C0
#define EMI_BUS_CFG2_TDWR_MASK                                           0x0300
#define EMI_BUS_CFG2_TCHR_MASK                                           0x0C00
#define EMI_BUS_CFG2_CS0_CACHE_DIS_MASK                                  0x1000
#define EMI_BUS_CFG2_CS1_CACHE_DIS_MASK                                  0x2000

   struct
   {
      unsigned int   tras                              :  2 ;  /*    [15:14] */
      unsigned int   trcd                              :  2 ;  /*    [13:12] */
      unsigned int   trp                               :  2 ;  /*    [11:10] */
      unsigned int   burst_size                        :  4 ;  /*      [9:6] */
      unsigned int   __fill0                           :  2 ;  /*      [5:4] */
      unsigned int   cas_latency                       :  2 ;  /*      [3:2] */
      unsigned int   cs1_en                            :  1 ;  /*        [1] */
      unsigned int   cs0_en                            :  1 ;  /*        [0] */
   } sdram_cfg ;

#define EMI_SDRAM_CFG_CS0_EN_MASK                                        0x0001
#define EMI_SDRAM_CFG_CS1_EN_MASK                                        0x0002
#define EMI_SDRAM_CFG_CAS_LATENCY_MASK                                   0x000C
#define EMI_SDRAM_CFG_BURST_SIZE_MASK                                    0x03C0
#define EMI_SDRAM_CFG_TRP_MASK                                           0x0C00
#define EMI_SDRAM_CFG_TRCD_MASK                                          0x3000
#define EMI_SDRAM_CFG_TRAS_MASK                                          0xC000

   struct
   {
      unsigned int   __fill0                           :  2 ;  /*    [15:14] */
      unsigned int   adr                               : 14 ;  /*     [13:0] */
   } sdram_cust_adr ;

#define EMI_SDRAM_CUST_ADR_ADR_MASK                                      0x3FFF

   struct
   {
      unsigned int   idle_dis                          :  1 ;  /*       [15] */
      unsigned int   cmd3                              :  5 ;  /*    [14:10] */
      unsigned int   cmd2                              :  5 ;  /*      [9:5] */
      unsigned int   cmd1                              :  5 ;  /*      [4:0] */
   } sdram_cust_cmd ;

#define EMI_SDRAM_CUST_CMD_CMD1_MASK                                     0x001F
#define EMI_SDRAM_CUST_CMD_CMD2_MASK                                     0x03E0
#define EMI_SDRAM_CUST_CMD_CMD3_MASK                                     0x7C00
#define EMI_SDRAM_CUST_CMD_IDLE_DIS_MASK                                 0x8000

   struct
   {
      unsigned int   __fill0                           :  1 ;  /*       [15] */
      unsigned int   init_pre_ch                       :  1 ;  /*       [14] */
      unsigned int   trfc                              :  2 ;  /*    [13:12] */
      unsigned int   per                               : 12 ;  /*     [11:0] */
   } sdram_refr_per ;

#define EMI_SDRAM_REFR_PER_PER_MASK                                      0x0FFF
#define EMI_SDRAM_REFR_PER_TRFC_MASK                                     0x3000
#define EMI_SDRAM_REFR_PER_INIT_PRE_CH_MASK                              0x4000

   struct
   {
      unsigned int   __fill0                           :  6 ;  /*    [15:10] */
      unsigned int   cnt                               : 10 ;  /*      [9:0] */
   } sdram_refr_cnt ;

#define EMI_SDRAM_REFR_CNT_CNT_MASK                                      0x03FF

} ;



struct __intact_fd_tag
{
   struct
   {
      unsigned int   __fill0                           :  1 ;  /*       [15] */
      unsigned int   down_clk_act_ev                   :  1 ;  /*       [14] */
      unsigned int   down_tx_guard_dis                 :  1 ;  /*       [13] */
      unsigned int   down_tx_ack_olap_en               :  1 ;  /*       [12] */
      unsigned int   __fill1                           :  1 ;  /*       [11] */
      unsigned int   up_clk_act_ev                     :  1 ;  /*       [10] */
      unsigned int   up_tx_guard_dis                   :  1 ;  /*        [9] */
      unsigned int   up_tx_ack_olap_en                 :  1 ;  /*        [8] */
      unsigned int   rx_host_purge_dis                 :  1 ;  /*        [7] */
      unsigned int   __fill2                           :  2 ;  /*      [6:5] */
      unsigned int   adr                               :  5 ;  /*      [4:0] */
   } cfg ;

#define INTACT_CFG_UP_CLK_ACT_EV_FULL                          1
#define INTACT_CFG_UP_CLK_ACT_EV_PRESCALED                     0

#define INTACT_CFG_DOWN_CLK_ACT_EV_FULL                        1
#define INTACT_CFG_DOWN_CLK_ACT_EV_PRESCALED                   0

#define INTACT_CFG_ADR_MASK                                              0x001F
#define INTACT_CFG_RX_HOST_PURGE_DIS_MASK                                0x0080
#define INTACT_CFG_UP_TX_ACK_OLAP_EN_MASK                                0x0100
#define INTACT_CFG_UP_TX_GUARD_DIS_MASK                                  0x0200
#define INTACT_CFG_UP_CLK_ACT_EV_MASK                                    0x0400
#define INTACT_CFG_DOWN_TX_ACK_OLAP_EN_MASK                              0x1000
#define INTACT_CFG_DOWN_TX_GUARD_DIS_MASK                                0x2000
#define INTACT_CFG_DOWN_CLK_ACT_EV_MASK                                  0x4000

   struct
   {
      unsigned int   __fill0                           : 11 ;  /*     [15:5] */
      unsigned int   isoc_en                           :  1 ;  /*        [4] */
      unsigned int   rx_auto_hdr                       :  1 ;  /*        [3] */
      unsigned int   tx_auto_hdr                       :  1 ;  /*        [2] */
      unsigned int   endian                            :  1 ;  /*        [1] */
      unsigned int   __fill1                           :  1 ;  /*        [0] */
   } cpu_cfg ;

#define INTACT_CPU_CFG_ENDIAN_BIG                              1
#define INTACT_CPU_CFG_ENDIAN_LITTLE                           0

#define INTACT_CPU_CFG_ENDIAN_MASK                                       0x0002
#define INTACT_CPU_CFG_TX_AUTO_HDR_MASK                                  0x0004
#define INTACT_CPU_CFG_RX_AUTO_HDR_MASK                                  0x0008
#define INTACT_CPU_CFG_ISOC_EN_MASK                                      0x0010

   struct
   {
      unsigned int   __fill0                           : 11 ;  /*     [15:5] */
      unsigned int   isoc_en                           :  1 ;  /*        [4] */
      unsigned int   rx_auto_hdr                       :  1 ;  /*        [3] */
      unsigned int   tx_auto_hdr                       :  1 ;  /*        [2] */
      unsigned int   endian                            :  1 ;  /*        [1] */
      unsigned int   dir                               :  1 ;  /*        [0] */
   } dma_cfg ;

#define INTACT_DMA_CFG_DIR_TX                                  1
#define INTACT_DMA_CFG_DIR_RX                                  0

#define INTACT_DMA_CFG_ENDIAN_BIG                              1
#define INTACT_DMA_CFG_ENDIAN_LITTLE                           0

#define INTACT_DMA_CFG_DIR_MASK                                          0x0001
#define INTACT_DMA_CFG_ENDIAN_MASK                                       0x0002
#define INTACT_DMA_CFG_TX_AUTO_HDR_MASK                                  0x0004
#define INTACT_DMA_CFG_RX_AUTO_HDR_MASK                                  0x0008
#define INTACT_DMA_CFG_ISOC_EN_MASK                                      0x0010

   struct
   {
      unsigned int   __fill0                           : 11 ;  /*     [15:5] */
      unsigned int   isoc_en                           :  1 ;  /*        [4] */
      unsigned int   rx_auto_hdr                       :  1 ;  /*        [3] */
      unsigned int   tx_auto_hdr                       :  1 ;  /*        [2] */
      unsigned int   endian                            :  1 ;  /*        [1] */
      unsigned int   dir                               :  1 ;  /*        [0] */
   } str_cfg ;

#define INTACT_STR_CFG_DIR_TX                                  1
#define INTACT_STR_CFG_DIR_RX                                  0

#define INTACT_STR_CFG_ENDIAN_BIG                              1
#define INTACT_STR_CFG_ENDIAN_LITTLE                           0

#define INTACT_STR_CFG_DIR_MASK                                          0x0001
#define INTACT_STR_CFG_ENDIAN_MASK                                       0x0002
#define INTACT_STR_CFG_TX_AUTO_HDR_MASK                                  0x0004
#define INTACT_STR_CFG_RX_AUTO_HDR_MASK                                  0x0008
#define INTACT_STR_CFG_ISOC_EN_MASK                                      0x0010

   struct
   {
      unsigned int   mask                              :  8 ;  /*     [15:8] */
      unsigned int   adr                               :  8 ;  /*      [7:0] */
   } cpu_rx_cfg ;

#define INTACT_CPU_RX_CFG_ADR_MASK                                       0x00FF
#define INTACT_CPU_RX_CFG_MASK_MASK                                      0xFF00

   struct
   {
      unsigned int   mask                              :  8 ;  /*     [15:8] */
      unsigned int   adr                               :  8 ;  /*      [7:0] */
   } dma_rx_cfg ;

#define INTACT_DMA_RX_CFG_ADR_MASK                                       0x00FF
#define INTACT_DMA_RX_CFG_MASK_MASK                                      0xFF00

   struct
   {
      unsigned int   mask                              :  8 ;  /*     [15:8] */
      unsigned int   adr                               :  8 ;  /*      [7:0] */
   } str_rx_cfg ;

#define INTACT_STR_RX_CFG_ADR_MASK                                       0x00FF
#define INTACT_STR_RX_CFG_MASK_MASK                                      0xFF00

   struct
   {
      unsigned int   msg                               :  2 ;  /*    [15:14] */
      unsigned int   port                              :  1 ;  /*       [13] */
      unsigned int   adr                               :  5 ;  /*     [12:8] */
      unsigned int   vci                               :  8 ;  /*      [7:0] */
   } cpu_tx_cfg ;

#define INTACT_CPU_TX_CFG_PORT_UP                              1
#define INTACT_CPU_TX_CFG_PORT_DOWN                            0

#define INTACT_CPU_TX_CFG_MSG_ACK                              3
#define INTACT_CPU_TX_CFG_MSG_NORMAL                           0

#define INTACT_CPU_TX_CFG_VCI_MASK                                       0x00FF
#define INTACT_CPU_TX_CFG_ADR_MASK                                       0x1F00
#define INTACT_CPU_TX_CFG_PORT_MASK                                      0x2000
#define INTACT_CPU_TX_CFG_MSG_MASK                                       0xC000

   struct
   {
      unsigned int   msg                               :  2 ;  /*    [15:14] */
      unsigned int   port                              :  1 ;  /*       [13] */
      unsigned int   adr                               :  5 ;  /*     [12:8] */
      unsigned int   vci                               :  8 ;  /*      [7:0] */
   } dma_tx_cfg ;

#define INTACT_DMA_TX_CFG_PORT_UP                              1
#define INTACT_DMA_TX_CFG_PORT_DOWN                            0

#define INTACT_DMA_TX_CFG_MSG_ACK                              3
#define INTACT_DMA_TX_CFG_MSG_NORMAL                           0

#define INTACT_DMA_TX_CFG_VCI_MASK                                       0x00FF
#define INTACT_DMA_TX_CFG_ADR_MASK                                       0x1F00
#define INTACT_DMA_TX_CFG_PORT_MASK                                      0x2000
#define INTACT_DMA_TX_CFG_MSG_MASK                                       0xC000

   struct
   {
      unsigned int   msg                               :  2 ;  /*    [15:14] */
      unsigned int   port                              :  1 ;  /*       [13] */
      unsigned int   adr                               :  5 ;  /*     [12:8] */
      unsigned int   vci                               :  8 ;  /*      [7:0] */
   } str_tx_cfg ;

#define INTACT_STR_TX_CFG_PORT_UP                              1
#define INTACT_STR_TX_CFG_PORT_DOWN                            0

#define INTACT_STR_TX_CFG_MSG_ACK                              3
#define INTACT_STR_TX_CFG_MSG_NORMAL                           0

#define INTACT_STR_TX_CFG_VCI_MASK                                       0x00FF
#define INTACT_STR_TX_CFG_ADR_MASK                                       0x1F00
#define INTACT_STR_TX_CFG_PORT_MASK                                      0x2000
#define INTACT_STR_TX_CFG_MSG_MASK                                       0xC000

   struct
   {
      unsigned int   down_fwd_purge_ov_clr             :  1 ;  /*       [15] */
      unsigned int   down_fwd_purge_ov_set             :  1 ;  /*       [14] */
      unsigned int   down_fwd_purge_ov_dis             :  1 ;  /*       [13] */
      unsigned int   down_fwd_purge_ov_en              :  1 ;  /*       [12] */
      unsigned int   __fill0                           :  1 ;  /*       [11] */
      unsigned int   up_fwd_purge_ov_clr               :  1 ;  /*       [10] */
      unsigned int   up_fwd_purge_ov_set               :  1 ;  /*        [9] */
      unsigned int   up_fwd_purge_ov_dis               :  1 ;  /*        [8] */
      unsigned int   up_fwd_purge_ov_en                :  1 ;  /*        [7] */
      unsigned int   __fill1                           :  1 ;  /*        [6] */
      unsigned int   str_dis                           :  1 ;  /*        [5] */
      unsigned int   str_en                            :  1 ;  /*        [4] */
      unsigned int   dma_dis                           :  1 ;  /*        [3] */
      unsigned int   dma_en                            :  1 ;  /*        [2] */
      unsigned int   cpu_dis                           :  1 ;  /*        [1] */
      unsigned int   cpu_en                            :  1 ;  /*        [0] */
   } ctrl ;

#define INTACT_CTRL_CPU_EN_MASK                                          0x0001
#define INTACT_CTRL_CPU_DIS_MASK                                         0x0002
#define INTACT_CTRL_DMA_EN_MASK                                          0x0004
#define INTACT_CTRL_DMA_DIS_MASK                                         0x0008
#define INTACT_CTRL_STR_EN_MASK                                          0x0010
#define INTACT_CTRL_STR_DIS_MASK                                         0x0020
#define INTACT_CTRL_UP_FWD_PURGE_OV_EN_MASK                              0x0080
#define INTACT_CTRL_UP_FWD_PURGE_OV_DIS_MASK                             0x0100
#define INTACT_CTRL_UP_FWD_PURGE_OV_SET_MASK                             0x0200
#define INTACT_CTRL_UP_FWD_PURGE_OV_CLR_MASK                             0x0400
#define INTACT_CTRL_DOWN_FWD_PURGE_OV_EN_MASK                            0x1000
#define INTACT_CTRL_DOWN_FWD_PURGE_OV_DIS_MASK                           0x2000
#define INTACT_CTRL_DOWN_FWD_PURGE_OV_SET_MASK                           0x4000
#define INTACT_CTRL_DOWN_FWD_PURGE_OV_CLR_MASK                           0x8000

   struct
   {
      unsigned int   __fill0                           : 10 ;  /*     [15:6] */
      unsigned int   down_clk_en                       :  1 ;  /*        [5] */
      unsigned int   down_clk_dis                      :  1 ;  /*        [4] */
      unsigned int   __fill1                           :  2 ;  /*      [3:2] */
      unsigned int   up_clk_en                         :  1 ;  /*        [1] */
      unsigned int   up_clk_dis                        :  1 ;  /*        [0] */
   } clk_ctrl ;

#define INTACT_CLK_CTRL_UP_CLK_DIS_MASK                                  0x0001
#define INTACT_CLK_CTRL_UP_CLK_EN_MASK                                   0x0002
#define INTACT_CLK_CTRL_DOWN_CLK_DIS_MASK                                0x0010
#define INTACT_CLK_CTRL_DOWN_CLK_EN_MASK                                 0x0020

   struct
   {
      unsigned int   __fill0                           :  2 ;  /*    [15:14] */
      unsigned int   down_rx_act                       :  1 ;  /*       [13] */
      unsigned int   down_tx_act                       :  1 ;  /*       [12] */
      unsigned int   __fill1                           :  2 ;  /*    [11:10] */
      unsigned int   up_rx_act                         :  1 ;  /*        [9] */
      unsigned int   up_tx_act                         :  1 ;  /*        [8] */
      unsigned int   __fill2                           :  5 ;  /*      [7:3] */
      unsigned int   str_act                           :  1 ;  /*        [2] */
      unsigned int   dma_act                           :  1 ;  /*        [1] */
      unsigned int   cpu_act                           :  1 ;  /*        [0] */
   } sts ;

#define INTACT_STS_CPU_ACT_MASK                                          0x0001
#define INTACT_STS_DMA_ACT_MASK                                          0x0002
#define INTACT_STS_STR_ACT_MASK                                          0x0004
#define INTACT_STS_UP_TX_ACT_MASK                                        0x0100
#define INTACT_STS_UP_RX_ACT_MASK                                        0x0200
#define INTACT_STS_DOWN_TX_ACT_MASK                                      0x1000
#define INTACT_STS_DOWN_RX_ACT_MASK                                      0x2000

   struct
   {
      unsigned int   act                               :  1 ;  /*       [15] */
      unsigned int   valid                             :  1 ;  /*       [14] */
      unsigned int   __fill0                           :  2 ;  /*    [13:12] */
      unsigned int   cnt                               : 12 ;  /*     [11:0] */
   } up_clk ;

#define INTACT_UP_CLK_CNT_MASK                                           0x0FFF
#define INTACT_UP_CLK_VALID_MASK                                         0x4000
#define INTACT_UP_CLK_ACT_MASK                                           0x8000

   struct
   {
      unsigned int   act                               :  1 ;  /*       [15] */
      unsigned int   valid                             :  1 ;  /*       [14] */
      unsigned int   __fill0                           :  2 ;  /*    [13:12] */
      unsigned int   cnt                               : 12 ;  /*     [11:0] */
   } down_clk ;

#define INTACT_DOWN_CLK_CNT_MASK                                         0x0FFF
#define INTACT_DOWN_CLK_VALID_MASK                                       0x4000
#define INTACT_DOWN_CLK_ACT_MASK                                         0x8000

   struct
   {
      unsigned int   dma_done                          :  1 ;  /*       [15] */
      unsigned int   dma_rdy                           :  1 ;  /*       [14] */
      unsigned int   __fill0                           :  1 ;  /*       [13] */
      unsigned int   str_rx_ufl                        :  1 ;  /*       [12] */
      unsigned int   dma_rx_ufl                        :  1 ;  /*       [11] */
      unsigned int   cpu_rx_ufl                        :  1 ;  /*       [10] */
      unsigned int   cpu_rx_ofl                        :  1 ;  /*        [9] */
      unsigned int   cpu_tx_ofl                        :  1 ;  /*        [8] */
      unsigned int   cpu_rx_rdy                        :  1 ;  /*        [7] */
      unsigned int   cpu_tx_rdy                        :  1 ;  /*        [6] */
      unsigned int   down_fwd_purge                    :  1 ;  /*        [5] */
      unsigned int   down_host_purge                   :  1 ;  /*        [4] */
      unsigned int   down_clk_inact                    :  1 ;  /*        [3] */
      unsigned int   up_fwd_purge                      :  1 ;  /*        [2] */
      unsigned int   up_host_purge                     :  1 ;  /*        [1] */
      unsigned int   up_clk_inact                      :  1 ;  /*        [0] */
   } int_sts ;

#define INTACT_INT_STS_UP_CLK_INACT_MASK                                 0x0001
#define INTACT_INT_STS_UP_HOST_PURGE_MASK                                0x0002
#define INTACT_INT_STS_UP_FWD_PURGE_MASK                                 0x0004
#define INTACT_INT_STS_DOWN_CLK_INACT_MASK                               0x0008
#define INTACT_INT_STS_DOWN_HOST_PURGE_MASK                              0x0010
#define INTACT_INT_STS_DOWN_FWD_PURGE_MASK                               0x0020
#define INTACT_INT_STS_CPU_TX_RDY_MASK                                   0x0040
#define INTACT_INT_STS_CPU_RX_RDY_MASK                                   0x0080
#define INTACT_INT_STS_CPU_TX_OFL_MASK                                   0x0100
#define INTACT_INT_STS_CPU_RX_OFL_MASK                                   0x0200
#define INTACT_INT_STS_CPU_RX_UFL_MASK                                   0x0400
#define INTACT_INT_STS_DMA_RX_UFL_MASK                                   0x0800
#define INTACT_INT_STS_STR_RX_UFL_MASK                                   0x1000
#define INTACT_INT_STS_DMA_RDY_MASK                                      0x4000
#define INTACT_INT_STS_DMA_DONE_MASK                                     0x8000

   struct
   {
      unsigned int   dma_done                          :  1 ;  /*       [15] */
      unsigned int   dma_rdy                           :  1 ;  /*       [14] */
      unsigned int   __fill0                           :  1 ;  /*       [13] */
      unsigned int   str_rx_ufl                        :  1 ;  /*       [12] */
      unsigned int   dma_rx_ufl                        :  1 ;  /*       [11] */
      unsigned int   cpu_rx_ufl                        :  1 ;  /*       [10] */
      unsigned int   cpu_rx_ofl                        :  1 ;  /*        [9] */
      unsigned int   cpu_tx_ofl                        :  1 ;  /*        [8] */
      unsigned int   cpu_rx_rdy                        :  1 ;  /*        [7] */
      unsigned int   cpu_tx_rdy                        :  1 ;  /*        [6] */
      unsigned int   down_fwd_purge                    :  1 ;  /*        [5] */
      unsigned int   down_host_purge                   :  1 ;  /*        [4] */
      unsigned int   down_clk_inact                    :  1 ;  /*        [3] */
      unsigned int   up_fwd_purge                      :  1 ;  /*        [2] */
      unsigned int   up_host_purge                     :  1 ;  /*        [1] */
      unsigned int   up_clk_inact                      :  1 ;  /*        [0] */
   } int_en ;

#define INTACT_INT_EN_UP_CLK_INACT_MASK                                  0x0001
#define INTACT_INT_EN_UP_HOST_PURGE_MASK                                 0x0002
#define INTACT_INT_EN_UP_FWD_PURGE_MASK                                  0x0004
#define INTACT_INT_EN_DOWN_CLK_INACT_MASK                                0x0008
#define INTACT_INT_EN_DOWN_HOST_PURGE_MASK                               0x0010
#define INTACT_INT_EN_DOWN_FWD_PURGE_MASK                                0x0020
#define INTACT_INT_EN_CPU_TX_RDY_MASK                                    0x0040
#define INTACT_INT_EN_CPU_RX_RDY_MASK                                    0x0080
#define INTACT_INT_EN_CPU_TX_OFL_MASK                                    0x0100
#define INTACT_INT_EN_CPU_RX_OFL_MASK                                    0x0200
#define INTACT_INT_EN_CPU_RX_UFL_MASK                                    0x0400
#define INTACT_INT_EN_DMA_RX_UFL_MASK                                    0x0800
#define INTACT_INT_EN_STR_RX_UFL_MASK                                    0x1000
#define INTACT_INT_EN_DMA_RDY_MASK                                       0x4000
#define INTACT_INT_EN_DMA_DONE_MASK                                      0x8000

   unsigned int __fill0 ;                                      /*       FF93 */

   struct
   {
      unsigned int   dma_done                          :  1 ;  /*       [15] */
      unsigned int   dma_rdy                           :  1 ;  /*       [14] */
      unsigned int   __fill0                           :  1 ;  /*       [13] */
      unsigned int   str_rx_ufl                        :  1 ;  /*       [12] */
      unsigned int   dma_rx_ufl                        :  1 ;  /*       [11] */
      unsigned int   cpu_rx_ufl                        :  1 ;  /*       [10] */
      unsigned int   cpu_rx_ofl                        :  1 ;  /*        [9] */
      unsigned int   cpu_tx_ofl                        :  1 ;  /*        [8] */
      unsigned int   cpu_rx_rdy                        :  1 ;  /*        [7] */
      unsigned int   cpu_tx_rdy                        :  1 ;  /*        [6] */
      unsigned int   down_fwd_purge                    :  1 ;  /*        [5] */
      unsigned int   down_host_purge                   :  1 ;  /*        [4] */
      unsigned int   down_clk_inact                    :  1 ;  /*        [3] */
      unsigned int   up_fwd_purge                      :  1 ;  /*        [2] */
      unsigned int   up_host_purge                     :  1 ;  /*        [1] */
      unsigned int   up_clk_inact                      :  1 ;  /*        [0] */
   } int_dis ;

#define INTACT_INT_DIS_UP_CLK_INACT_MASK                                 0x0001
#define INTACT_INT_DIS_UP_HOST_PURGE_MASK                                0x0002
#define INTACT_INT_DIS_UP_FWD_PURGE_MASK                                 0x0004
#define INTACT_INT_DIS_DOWN_CLK_INACT_MASK                               0x0008
#define INTACT_INT_DIS_DOWN_HOST_PURGE_MASK                              0x0010
#define INTACT_INT_DIS_DOWN_FWD_PURGE_MASK                               0x0020
#define INTACT_INT_DIS_CPU_TX_RDY_MASK                                   0x0040
#define INTACT_INT_DIS_CPU_RX_RDY_MASK                                   0x0080
#define INTACT_INT_DIS_CPU_TX_OFL_MASK                                   0x0100
#define INTACT_INT_DIS_CPU_RX_OFL_MASK                                   0x0200
#define INTACT_INT_DIS_CPU_RX_UFL_MASK                                   0x0400
#define INTACT_INT_DIS_DMA_RX_UFL_MASK                                   0x0800
#define INTACT_INT_DIS_STR_RX_UFL_MASK                                   0x1000
#define INTACT_INT_DIS_DMA_RDY_MASK                                      0x4000
#define INTACT_INT_DIS_DMA_DONE_MASK                                     0x8000

   struct
   {
      unsigned int   dma_done                          :  1 ;  /*       [15] */
      unsigned int   __fill0                           :  2 ;  /*    [14:13] */
      unsigned int   str_rx_ufl                        :  1 ;  /*       [12] */
      unsigned int   dma_rx_ufl                        :  1 ;  /*       [11] */
      unsigned int   cpu_rx_ufl                        :  1 ;  /*       [10] */
      unsigned int   cpu_rx_ofl                        :  1 ;  /*        [9] */
      unsigned int   cpu_tx_ofl                        :  1 ;  /*        [8] */
      unsigned int   __fill1                           :  2 ;  /*      [7:6] */
      unsigned int   down_fwd_purge                    :  1 ;  /*        [5] */
      unsigned int   down_host_purge                   :  1 ;  /*        [4] */
      unsigned int   down_clk_inact                    :  1 ;  /*        [3] */
      unsigned int   up_fwd_purge                      :  1 ;  /*        [2] */
      unsigned int   up_host_purge                     :  1 ;  /*        [1] */
      unsigned int   up_clk_inact                      :  1 ;  /*        [0] */
   } int_clr ;

#define INTACT_INT_CLR_UP_CLK_INACT_MASK                                 0x0001
#define INTACT_INT_CLR_UP_HOST_PURGE_MASK                                0x0002
#define INTACT_INT_CLR_UP_FWD_PURGE_MASK                                 0x0004
#define INTACT_INT_CLR_DOWN_CLK_INACT_MASK                               0x0008
#define INTACT_INT_CLR_DOWN_HOST_PURGE_MASK                              0x0010
#define INTACT_INT_CLR_DOWN_FWD_PURGE_MASK                               0x0020
#define INTACT_INT_CLR_CPU_TX_OFL_MASK                                   0x0100
#define INTACT_INT_CLR_CPU_RX_OFL_MASK                                   0x0200
#define INTACT_INT_CLR_CPU_RX_UFL_MASK                                   0x0400
#define INTACT_INT_CLR_DMA_RX_UFL_MASK                                   0x0800
#define INTACT_INT_CLR_STR_RX_UFL_MASK                                   0x1000
#define INTACT_INT_CLR_DMA_DONE_MASK                                     0x8000

   struct
   {
      unsigned int   tx                                : 16 ;  /*     [15:0] */
   } tx ;

#define INTACT_TX_TX_MASK                                                0xFFFF

   struct
   {
      unsigned int   rx                                : 16 ;  /*     [15:0] */
   } rx ;

#define INTACT_RX_RX_MASK                                                0xFFFF

   struct
   {
      unsigned int   int_cfg                           :  1 ;  /*       [15] */
      unsigned int   wrap_en                           :  1 ;  /*       [14] */
      unsigned int   __fill0                           :  3 ;  /*    [13:11] */
      unsigned int   adr                               : 11 ;  /*     [10:0] */
   } xfr_cfg ;

#define INTACT_XFR_CFG_INT_CFG_HALF                            1
#define INTACT_XFR_CFG_INT_CFG_COMPLETE                        0

#define INTACT_XFR_CFG_ADR_MASK                                          0x07FF
#define INTACT_XFR_CFG_WRAP_EN_MASK                                      0x4000
#define INTACT_XFR_CFG_INT_CFG_MASK                                      0x8000

   struct
   {
      unsigned int   req                               :  1 ;  /*       [15] */
      unsigned int   rst                               :  1 ;  /*       [14] */
      unsigned int   stall_dis                         :  1 ;  /*       [13] */
      unsigned int   stall_en                          :  1 ;  /*       [12] */
      unsigned int   __fill0                           :  4 ;  /*     [11:8] */
      unsigned int   size                              :  8 ;  /*      [7:0] */
   } xfr_ctrl ;

#define INTACT_XFR_CTRL_SIZE_MASK                                        0x00FF
#define INTACT_XFR_CTRL_STALL_EN_MASK                                    0x1000
#define INTACT_XFR_CTRL_STALL_DIS_MASK                                   0x2000
#define INTACT_XFR_CTRL_RST_MASK                                         0x4000
#define INTACT_XFR_CTRL_REQ_MASK                                         0x8000

   struct
   {
      unsigned int   invalid                           :  1 ;  /*       [15] */
      unsigned int   __fill0                           :  7 ;  /*     [14:8] */
      unsigned int   value                             :  8 ;  /*      [7:0] */
   } xfr_sts ;

#define INTACT_XFR_STS_VALUE_MASK                                        0x00FF
#define INTACT_XFR_STS_INVALID_MASK                                      0x8000

} ;



struct __port_fd_tag
{
   struct
   {
      unsigned int   ver                               : 16 ;  /*     [15:0] */
   } ver ;

#define PORT_VER_VER_MASK                                                0xFFFF

   struct
   {
      unsigned int   f                                 :  2 ;  /*    [15:14] */
      unsigned int   e                                 :  2 ;  /*    [13:12] */
      unsigned int   d                                 :  2 ;  /*    [11:10] */
      unsigned int   c                                 :  3 ;  /*      [9:7] */
      unsigned int   b                                 :  3 ;  /*      [6:4] */
      unsigned int   a                                 :  4 ;  /*      [3:0] */
   } sel1 ;

#define PORT_SEL1_A_MASK                                                 0x000F
#define PORT_SEL1_B_MASK                                                 0x0070
#define PORT_SEL1_C_MASK                                                 0x0380
#define PORT_SEL1_D_MASK                                                 0x0C00
#define PORT_SEL1_E_MASK                                                 0x3000
#define PORT_SEL1_F_MASK                                                 0xC000

   struct
   {
      unsigned int   __fill0                           :  4 ;  /*    [15:12] */
      unsigned int   l                                 :  2 ;  /*    [11:10] */
      unsigned int   k                                 :  2 ;  /*      [9:8] */
      unsigned int   j                                 :  2 ;  /*      [7:6] */
      unsigned int   i                                 :  2 ;  /*      [5:4] */
      unsigned int   h                                 :  2 ;  /*      [3:2] */
      unsigned int   g                                 :  2 ;  /*      [1:0] */
   } sel2 ;

#define PORT_SEL2_G_MASK                                                 0x0003
#define PORT_SEL2_H_MASK                                                 0x000C
#define PORT_SEL2_I_MASK                                                 0x0030
#define PORT_SEL2_J_MASK                                                 0x00C0
#define PORT_SEL2_K_MASK                                                 0x0300
#define PORT_SEL2_L_MASK                                                 0x0C00

   struct
   {
      unsigned int   __fill0                           :  4 ;  /*    [15:12] */
      unsigned int   l                                 :  1 ;  /*       [11] */
      unsigned int   k                                 :  1 ;  /*       [10] */
      unsigned int   j                                 :  1 ;  /*        [9] */
      unsigned int   i                                 :  1 ;  /*        [8] */
      unsigned int   h                                 :  1 ;  /*        [7] */
      unsigned int   g                                 :  1 ;  /*        [6] */
      unsigned int   f                                 :  1 ;  /*        [5] */
      unsigned int   e                                 :  1 ;  /*        [4] */
      unsigned int   d                                 :  1 ;  /*        [3] */
      unsigned int   c                                 :  1 ;  /*        [2] */
      unsigned int   b                                 :  1 ;  /*        [1] */
      unsigned int   a                                 :  1 ;  /*        [0] */
   } en ;

#define PORT_EN_A_MASK                                                   0x0001
#define PORT_EN_B_MASK                                                   0x0002
#define PORT_EN_C_MASK                                                   0x0004
#define PORT_EN_D_MASK                                                   0x0008
#define PORT_EN_E_MASK                                                   0x0010
#define PORT_EN_F_MASK                                                   0x0020
#define PORT_EN_G_MASK                                                   0x0040
#define PORT_EN_H_MASK                                                   0x0080
#define PORT_EN_I_MASK                                                   0x0100
#define PORT_EN_J_MASK                                                   0x0200
#define PORT_EN_K_MASK                                                   0x0400
#define PORT_EN_L_MASK                                                   0x0800

   struct
   {
      unsigned int   __fill0                           :  4 ;  /*    [15:12] */
      unsigned int   l                                 :  1 ;  /*       [11] */
      unsigned int   k                                 :  1 ;  /*       [10] */
      unsigned int   j                                 :  1 ;  /*        [9] */
      unsigned int   i                                 :  1 ;  /*        [8] */
      unsigned int   h                                 :  1 ;  /*        [7] */
      unsigned int   g                                 :  1 ;  /*        [6] */
      unsigned int   f                                 :  1 ;  /*        [5] */
      unsigned int   e                                 :  1 ;  /*        [4] */
      unsigned int   d                                 :  1 ;  /*        [3] */
      unsigned int   c                                 :  1 ;  /*        [2] */
      unsigned int   b                                 :  1 ;  /*        [1] */
      unsigned int   a                                 :  1 ;  /*        [0] */
   } dis ;

#define PORT_DIS_A_MASK                                                  0x0001
#define PORT_DIS_B_MASK                                                  0x0002
#define PORT_DIS_C_MASK                                                  0x0004
#define PORT_DIS_D_MASK                                                  0x0008
#define PORT_DIS_E_MASK                                                  0x0010
#define PORT_DIS_F_MASK                                                  0x0020
#define PORT_DIS_G_MASK                                                  0x0040
#define PORT_DIS_H_MASK                                                  0x0080
#define PORT_DIS_I_MASK                                                  0x0100
#define PORT_DIS_J_MASK                                                  0x0200
#define PORT_DIS_K_MASK                                                  0x0400
#define PORT_DIS_L_MASK                                                  0x0800

} ;



struct __io_fd_tag
{
   struct
   {
      unsigned int   __fill0                           :  4 ;  /*    [15:12] */
      unsigned int   b_mode                            :  2 ;  /*    [11:10] */
      unsigned int   b_dis                             :  1 ;  /*        [9] */
      unsigned int   b_en                              :  1 ;  /*        [8] */
      unsigned int   __fill1                           :  4 ;  /*      [7:4] */
      unsigned int   a_mode                            :  2 ;  /*      [3:2] */
      unsigned int   a_dis                             :  1 ;  /*        [1] */
      unsigned int   a_en                              :  1 ;  /*        [0] */
   } p_cfg ;

#define IO_P_CFG_A_MODE_DRIVEN                                 2
#define IO_P_CFG_A_MODE_OPEN_DRAIN                             1

#define IO_P_CFG_B_MODE_DRIVEN                                 2
#define IO_P_CFG_B_MODE_OPEN_DRAIN                             1

#define IO_P_CFG_A_EN_MASK                                               0x0001
#define IO_P_CFG_A_DIS_MASK                                              0x0002
#define IO_P_CFG_A_MODE_MASK                                             0x000C
#define IO_P_CFG_B_EN_MASK                                               0x0100
#define IO_P_CFG_B_DIS_MASK                                              0x0200
#define IO_P_CFG_B_MODE_MASK                                             0x0C00

   struct
   {
      unsigned int   pa_out                            : 16 ;  /*     [15:0] */
   } pa_out ;

#define IO_PA_OUT_PA_OUT_MASK                                            0xFFFF

   struct
   {
      unsigned int   pb_out                            : 16 ;  /*     [15:0] */
   } pb_out ;

#define IO_PB_OUT_PB_OUT_MASK                                            0xFFFF

   struct
   {
      unsigned int   pa_in                             : 16 ;  /*     [15:0] */
   } pa_in ;

#define IO_PA_IN_PA_IN_MASK                                              0xFFFF

   struct
   {
      unsigned int   pb_in                             : 16 ;  /*     [15:0] */
   } pb_in ;

#define IO_PB_IN_PB_IN_MASK                                              0xFFFF

   struct
   {
      unsigned int   wake_en3                          :  1 ;  /*       [15] */
      unsigned int   int3                              :  3 ;  /*    [14:12] */
      unsigned int   wake_en2                          :  1 ;  /*       [11] */
      unsigned int   int2                              :  3 ;  /*     [10:8] */
      unsigned int   wake_en1                          :  1 ;  /*        [7] */
      unsigned int   int1                              :  3 ;  /*      [6:4] */
      unsigned int   wake_en0                          :  1 ;  /*        [3] */
      unsigned int   int0                              :  3 ;  /*      [2:0] */
   } gp0_3_cfg ;

#define IO_GP0_3_CFG_INT0_EDGE                                 7
#define IO_GP0_3_CFG_INT0_RISING                               5
#define IO_GP0_3_CFG_INT0_FALLING                              4
#define IO_GP0_3_CFG_INT0_HIGH                                 3
#define IO_GP0_3_CFG_INT0_LOW                                  2
#define IO_GP0_3_CFG_INT0_NONE                                 0

#define IO_GP0_3_CFG_INT1_EDGE                                 7
#define IO_GP0_3_CFG_INT1_RISING                               5
#define IO_GP0_3_CFG_INT1_FALLING                              4
#define IO_GP0_3_CFG_INT1_HIGH                                 3
#define IO_GP0_3_CFG_INT1_LOW                                  2
#define IO_GP0_3_CFG_INT1_NONE                                 0

#define IO_GP0_3_CFG_INT2_EDGE                                 7
#define IO_GP0_3_CFG_INT2_RISING                               5
#define IO_GP0_3_CFG_INT2_FALLING                              4
#define IO_GP0_3_CFG_INT2_HIGH                                 3
#define IO_GP0_3_CFG_INT2_LOW                                  2
#define IO_GP0_3_CFG_INT2_NONE                                 0

#define IO_GP0_3_CFG_INT3_EDGE                                 7
#define IO_GP0_3_CFG_INT3_RISING                               5
#define IO_GP0_3_CFG_INT3_FALLING                              4
#define IO_GP0_3_CFG_INT3_HIGH                                 3
#define IO_GP0_3_CFG_INT3_LOW                                  2
#define IO_GP0_3_CFG_INT3_NONE                                 0

#define IO_GP0_3_CFG_INT0_MASK                                           0x0007
#define IO_GP0_3_CFG_WAKE_EN0_MASK                                       0x0008
#define IO_GP0_3_CFG_INT1_MASK                                           0x0070
#define IO_GP0_3_CFG_WAKE_EN1_MASK                                       0x0080
#define IO_GP0_3_CFG_INT2_MASK                                           0x0700
#define IO_GP0_3_CFG_WAKE_EN2_MASK                                       0x0800
#define IO_GP0_3_CFG_INT3_MASK                                           0x7000
#define IO_GP0_3_CFG_WAKE_EN3_MASK                                       0x8000

   struct
   {
      unsigned int   wake_en7                          :  1 ;  /*       [15] */
      unsigned int   int7                              :  3 ;  /*    [14:12] */
      unsigned int   wake_en6                          :  1 ;  /*       [11] */
      unsigned int   int6                              :  3 ;  /*     [10:8] */
      unsigned int   wake_en5                          :  1 ;  /*        [7] */
      unsigned int   int5                              :  3 ;  /*      [6:4] */
      unsigned int   wake_en4                          :  1 ;  /*        [3] */
      unsigned int   int4                              :  3 ;  /*      [2:0] */
   } gp4_7_cfg ;

#define IO_GP4_7_CFG_INT4_EDGE                                 7
#define IO_GP4_7_CFG_INT4_RISING                               5
#define IO_GP4_7_CFG_INT4_FALLING                              4
#define IO_GP4_7_CFG_INT4_HIGH                                 3
#define IO_GP4_7_CFG_INT4_LOW                                  2
#define IO_GP4_7_CFG_INT4_NONE                                 0

#define IO_GP4_7_CFG_INT5_EDGE                                 7
#define IO_GP4_7_CFG_INT5_RISING                               5
#define IO_GP4_7_CFG_INT5_FALLING                              4
#define IO_GP4_7_CFG_INT5_HIGH                                 3
#define IO_GP4_7_CFG_INT5_LOW                                  2
#define IO_GP4_7_CFG_INT5_NONE                                 0

#define IO_GP4_7_CFG_INT6_EDGE                                 7
#define IO_GP4_7_CFG_INT6_RISING                               5
#define IO_GP4_7_CFG_INT6_FALLING                              4
#define IO_GP4_7_CFG_INT6_HIGH                                 3
#define IO_GP4_7_CFG_INT6_LOW                                  2
#define IO_GP4_7_CFG_INT6_NONE                                 0

#define IO_GP4_7_CFG_INT7_EDGE                                 7
#define IO_GP4_7_CFG_INT7_RISING                               5
#define IO_GP4_7_CFG_INT7_FALLING                              4
#define IO_GP4_7_CFG_INT7_HIGH                                 3
#define IO_GP4_7_CFG_INT7_LOW                                  2
#define IO_GP4_7_CFG_INT7_NONE                                 0

#define IO_GP4_7_CFG_INT4_MASK                                           0x0007
#define IO_GP4_7_CFG_WAKE_EN4_MASK                                       0x0008
#define IO_GP4_7_CFG_INT5_MASK                                           0x0070
#define IO_GP4_7_CFG_WAKE_EN5_MASK                                       0x0080
#define IO_GP4_7_CFG_INT6_MASK                                           0x0700
#define IO_GP4_7_CFG_WAKE_EN6_MASK                                       0x0800
#define IO_GP4_7_CFG_INT7_MASK                                           0x7000
#define IO_GP4_7_CFG_WAKE_EN7_MASK                                       0x8000

   struct
   {
      unsigned int   wake_en11                         :  1 ;  /*       [15] */
      unsigned int   int11                             :  3 ;  /*    [14:12] */
      unsigned int   wake_en10                         :  1 ;  /*       [11] */
      unsigned int   int10                             :  3 ;  /*     [10:8] */
      unsigned int   wake_en9                          :  1 ;  /*        [7] */
      unsigned int   int9                              :  3 ;  /*      [6:4] */
      unsigned int   wake_en8                          :  1 ;  /*        [3] */
      unsigned int   int8                              :  3 ;  /*      [2:0] */
   } gp8_11_cfg ;

#define IO_GP8_11_CFG_INT8_EDGE                                7
#define IO_GP8_11_CFG_INT8_RISING                              5
#define IO_GP8_11_CFG_INT8_FALLING                             4
#define IO_GP8_11_CFG_INT8_HIGH                                3
#define IO_GP8_11_CFG_INT8_LOW                                 2
#define IO_GP8_11_CFG_INT8_NONE                                0

#define IO_GP8_11_CFG_INT9_EDGE                                7
#define IO_GP8_11_CFG_INT9_RISING                              5
#define IO_GP8_11_CFG_INT9_FALLING                             4
#define IO_GP8_11_CFG_INT9_HIGH                                3
#define IO_GP8_11_CFG_INT9_LOW                                 2
#define IO_GP8_11_CFG_INT9_NONE                                0

#define IO_GP8_11_CFG_INT10_EDGE                               7
#define IO_GP8_11_CFG_INT10_RISING                             5
#define IO_GP8_11_CFG_INT10_FALLING                            4
#define IO_GP8_11_CFG_INT10_HIGH                               3
#define IO_GP8_11_CFG_INT10_LOW                                2
#define IO_GP8_11_CFG_INT10_NONE                               0

#define IO_GP8_11_CFG_INT11_EDGE                               7
#define IO_GP8_11_CFG_INT11_RISING                             5
#define IO_GP8_11_CFG_INT11_FALLING                            4
#define IO_GP8_11_CFG_INT11_HIGH                               3
#define IO_GP8_11_CFG_INT11_LOW                                2
#define IO_GP8_11_CFG_INT11_NONE                               0

#define IO_GP8_11_CFG_INT8_MASK                                          0x0007
#define IO_GP8_11_CFG_WAKE_EN8_MASK                                      0x0008
#define IO_GP8_11_CFG_INT9_MASK                                          0x0070
#define IO_GP8_11_CFG_WAKE_EN9_MASK                                      0x0080
#define IO_GP8_11_CFG_INT10_MASK                                         0x0700
#define IO_GP8_11_CFG_WAKE_EN10_MASK                                     0x0800
#define IO_GP8_11_CFG_INT11_MASK                                         0x7000
#define IO_GP8_11_CFG_WAKE_EN11_MASK                                     0x8000

   struct
   {
      unsigned int   wake_en15                         :  1 ;  /*       [15] */
      unsigned int   int15                             :  3 ;  /*    [14:12] */
      unsigned int   wake_en14                         :  1 ;  /*       [11] */
      unsigned int   int14                             :  3 ;  /*     [10:8] */
      unsigned int   wake_en13                         :  1 ;  /*        [7] */
      unsigned int   int13                             :  3 ;  /*      [6:4] */
      unsigned int   wake_en12                         :  1 ;  /*        [3] */
      unsigned int   int12                             :  3 ;  /*      [2:0] */
   } gp12_15_cfg ;

#define IO_GP12_15_CFG_INT12_EDGE                              7
#define IO_GP12_15_CFG_INT12_RISING                            5
#define IO_GP12_15_CFG_INT12_FALLING                           4
#define IO_GP12_15_CFG_INT12_HIGH                              3
#define IO_GP12_15_CFG_INT12_LOW                               2
#define IO_GP12_15_CFG_INT12_NONE                              0

#define IO_GP12_15_CFG_INT13_EDGE                              7
#define IO_GP12_15_CFG_INT13_RISING                            5
#define IO_GP12_15_CFG_INT13_FALLING                           4
#define IO_GP12_15_CFG_INT13_HIGH                              3
#define IO_GP12_15_CFG_INT13_LOW                               2
#define IO_GP12_15_CFG_INT13_NONE                              0

#define IO_GP12_15_CFG_INT14_EDGE                              7
#define IO_GP12_15_CFG_INT14_RISING                            5
#define IO_GP12_15_CFG_INT14_FALLING                           4
#define IO_GP12_15_CFG_INT14_HIGH                              3
#define IO_GP12_15_CFG_INT14_LOW                               2
#define IO_GP12_15_CFG_INT14_NONE                              0

#define IO_GP12_15_CFG_INT15_EDGE                              7
#define IO_GP12_15_CFG_INT15_RISING                            5
#define IO_GP12_15_CFG_INT15_FALLING                           4
#define IO_GP12_15_CFG_INT15_HIGH                              3
#define IO_GP12_15_CFG_INT15_LOW                               2
#define IO_GP12_15_CFG_INT15_NONE                              0

#define IO_GP12_15_CFG_INT12_MASK                                        0x0007
#define IO_GP12_15_CFG_WAKE_EN12_MASK                                    0x0008
#define IO_GP12_15_CFG_INT13_MASK                                        0x0070
#define IO_GP12_15_CFG_WAKE_EN13_MASK                                    0x0080
#define IO_GP12_15_CFG_INT14_MASK                                        0x0700
#define IO_GP12_15_CFG_WAKE_EN14_MASK                                    0x0800
#define IO_GP12_15_CFG_INT15_MASK                                        0x7000
#define IO_GP12_15_CFG_WAKE_EN15_MASK                                    0x8000

   struct
   {
      unsigned int   wake_en19                         :  1 ;  /*       [15] */
      unsigned int   int19                             :  3 ;  /*    [14:12] */
      unsigned int   wake_en18                         :  1 ;  /*       [11] */
      unsigned int   int18                             :  3 ;  /*     [10:8] */
      unsigned int   wake_en17                         :  1 ;  /*        [7] */
      unsigned int   int17                             :  3 ;  /*      [6:4] */
      unsigned int   wake_en16                         :  1 ;  /*        [3] */
      unsigned int   int16                             :  3 ;  /*      [2:0] */
   } gp16_19_cfg ;

#define IO_GP16_19_CFG_INT16_EDGE                              7
#define IO_GP16_19_CFG_INT16_RISING                            5
#define IO_GP16_19_CFG_INT16_FALLING                           4
#define IO_GP16_19_CFG_INT16_HIGH                              3
#define IO_GP16_19_CFG_INT16_LOW                               2
#define IO_GP16_19_CFG_INT16_NONE                              0

#define IO_GP16_19_CFG_INT17_EDGE                              7
#define IO_GP16_19_CFG_INT17_RISING                            5
#define IO_GP16_19_CFG_INT17_FALLING                           4
#define IO_GP16_19_CFG_INT17_HIGH                              3
#define IO_GP16_19_CFG_INT17_LOW                               2
#define IO_GP16_19_CFG_INT17_NONE                              0

#define IO_GP16_19_CFG_INT18_EDGE                              7
#define IO_GP16_19_CFG_INT18_RISING                            5
#define IO_GP16_19_CFG_INT18_FALLING                           4
#define IO_GP16_19_CFG_INT18_HIGH                              3
#define IO_GP16_19_CFG_INT18_LOW                               2
#define IO_GP16_19_CFG_INT18_NONE                              0

#define IO_GP16_19_CFG_INT19_EDGE                              7
#define IO_GP16_19_CFG_INT19_RISING                            5
#define IO_GP16_19_CFG_INT19_FALLING                           4
#define IO_GP16_19_CFG_INT19_HIGH                              3
#define IO_GP16_19_CFG_INT19_LOW                               2
#define IO_GP16_19_CFG_INT19_NONE                              0

#define IO_GP16_19_CFG_INT16_MASK                                        0x0007
#define IO_GP16_19_CFG_WAKE_EN16_MASK                                    0x0008
#define IO_GP16_19_CFG_INT17_MASK                                        0x0070
#define IO_GP16_19_CFG_WAKE_EN17_MASK                                    0x0080
#define IO_GP16_19_CFG_INT18_MASK                                        0x0700
#define IO_GP16_19_CFG_WAKE_EN18_MASK                                    0x0800
#define IO_GP16_19_CFG_INT19_MASK                                        0x7000
#define IO_GP16_19_CFG_WAKE_EN19_MASK                                    0x8000

   struct
   {
      unsigned int   wake_en23                         :  1 ;  /*       [15] */
      unsigned int   int23                             :  3 ;  /*    [14:12] */
      unsigned int   wake_en22                         :  1 ;  /*       [11] */
      unsigned int   int22                             :  3 ;  /*     [10:8] */
      unsigned int   wake_en21                         :  1 ;  /*        [7] */
      unsigned int   int21                             :  3 ;  /*      [6:4] */
      unsigned int   wake_en20                         :  1 ;  /*        [3] */
      unsigned int   int20                             :  3 ;  /*      [2:0] */
   } gp20_23_cfg ;

#define IO_GP20_23_CFG_INT20_EDGE                              7
#define IO_GP20_23_CFG_INT20_RISING                            5
#define IO_GP20_23_CFG_INT20_FALLING                           4
#define IO_GP20_23_CFG_INT20_HIGH                              3
#define IO_GP20_23_CFG_INT20_LOW                               2
#define IO_GP20_23_CFG_INT20_NONE                              0

#define IO_GP20_23_CFG_INT21_EDGE                              7
#define IO_GP20_23_CFG_INT21_RISING                            5
#define IO_GP20_23_CFG_INT21_FALLING                           4
#define IO_GP20_23_CFG_INT21_HIGH                              3
#define IO_GP20_23_CFG_INT21_LOW                               2
#define IO_GP20_23_CFG_INT21_NONE                              0

#define IO_GP20_23_CFG_INT22_EDGE                              7
#define IO_GP20_23_CFG_INT22_RISING                            5
#define IO_GP20_23_CFG_INT22_FALLING                           4
#define IO_GP20_23_CFG_INT22_HIGH                              3
#define IO_GP20_23_CFG_INT22_LOW                               2
#define IO_GP20_23_CFG_INT22_NONE                              0

#define IO_GP20_23_CFG_INT23_EDGE                              7
#define IO_GP20_23_CFG_INT23_RISING                            5
#define IO_GP20_23_CFG_INT23_FALLING                           4
#define IO_GP20_23_CFG_INT23_HIGH                              3
#define IO_GP20_23_CFG_INT23_LOW                               2
#define IO_GP20_23_CFG_INT23_NONE                              0

#define IO_GP20_23_CFG_INT20_MASK                                        0x0007
#define IO_GP20_23_CFG_WAKE_EN20_MASK                                    0x0008
#define IO_GP20_23_CFG_INT21_MASK                                        0x0070
#define IO_GP20_23_CFG_WAKE_EN21_MASK                                    0x0080
#define IO_GP20_23_CFG_INT22_MASK                                        0x0700
#define IO_GP20_23_CFG_WAKE_EN22_MASK                                    0x0800
#define IO_GP20_23_CFG_INT23_MASK                                        0x7000
#define IO_GP20_23_CFG_WAKE_EN23_MASK                                    0x8000

   struct
   {
      unsigned int   wake_en27                         :  1 ;  /*       [15] */
      unsigned int   int27                             :  3 ;  /*    [14:12] */
      unsigned int   wake_en26                         :  1 ;  /*       [11] */
      unsigned int   int26                             :  3 ;  /*     [10:8] */
      unsigned int   wake_en25                         :  1 ;  /*        [7] */
      unsigned int   int25                             :  3 ;  /*      [6:4] */
      unsigned int   wake_en24                         :  1 ;  /*        [3] */
      unsigned int   int24                             :  3 ;  /*      [2:0] */
   } gp24_27_cfg ;

#define IO_GP24_27_CFG_INT24_EDGE                              7
#define IO_GP24_27_CFG_INT24_RISING                            5
#define IO_GP24_27_CFG_INT24_FALLING                           4
#define IO_GP24_27_CFG_INT24_HIGH                              3
#define IO_GP24_27_CFG_INT24_LOW                               2
#define IO_GP24_27_CFG_INT24_NONE                              0

#define IO_GP24_27_CFG_INT25_EDGE                              7
#define IO_GP24_27_CFG_INT25_RISING                            5
#define IO_GP24_27_CFG_INT25_FALLING                           4
#define IO_GP24_27_CFG_INT25_HIGH                              3
#define IO_GP24_27_CFG_INT25_LOW                               2
#define IO_GP24_27_CFG_INT25_NONE                              0

#define IO_GP24_27_CFG_INT26_EDGE                              7
#define IO_GP24_27_CFG_INT26_RISING                            5
#define IO_GP24_27_CFG_INT26_FALLING                           4
#define IO_GP24_27_CFG_INT26_HIGH                              3
#define IO_GP24_27_CFG_INT26_LOW                               2
#define IO_GP24_27_CFG_INT26_NONE                              0

#define IO_GP24_27_CFG_INT27_EDGE                              7
#define IO_GP24_27_CFG_INT27_RISING                            5
#define IO_GP24_27_CFG_INT27_FALLING                           4
#define IO_GP24_27_CFG_INT27_HIGH                              3
#define IO_GP24_27_CFG_INT27_LOW                               2
#define IO_GP24_27_CFG_INT27_NONE                              0

#define IO_GP24_27_CFG_INT24_MASK                                        0x0007
#define IO_GP24_27_CFG_WAKE_EN24_MASK                                    0x0008
#define IO_GP24_27_CFG_INT25_MASK                                        0x0070
#define IO_GP24_27_CFG_WAKE_EN25_MASK                                    0x0080
#define IO_GP24_27_CFG_INT26_MASK                                        0x0700
#define IO_GP24_27_CFG_WAKE_EN26_MASK                                    0x0800
#define IO_GP24_27_CFG_INT27_MASK                                        0x7000
#define IO_GP24_27_CFG_WAKE_EN27_MASK                                    0x8000

   struct
   {
      unsigned int   __fill0                           : 12 ;  /*     [15:4] */
      unsigned int   wake_en28                         :  1 ;  /*        [3] */
      unsigned int   int28                             :  3 ;  /*      [2:0] */
   } gp28_cfg ;

#define IO_GP28_CFG_INT28_EDGE                                 7
#define IO_GP28_CFG_INT28_RISING                               5
#define IO_GP28_CFG_INT28_FALLING                              4
#define IO_GP28_CFG_INT28_HIGH                                 3
#define IO_GP28_CFG_INT28_LOW                                  2
#define IO_GP28_CFG_INT28_NONE                                 0

#define IO_GP28_CFG_INT28_MASK                                           0x0007
#define IO_GP28_CFG_WAKE_EN28_MASK                                       0x0008

   struct
   {
      unsigned int   dis3                              :  1 ;  /*       [15] */
      unsigned int   en3                               :  1 ;  /*       [14] */
      unsigned int   clr3                              :  1 ;  /*       [13] */
      unsigned int   set3                              :  1 ;  /*       [12] */
      unsigned int   dis2                              :  1 ;  /*       [11] */
      unsigned int   en2                               :  1 ;  /*       [10] */
      unsigned int   clr2                              :  1 ;  /*        [9] */
      unsigned int   set2                              :  1 ;  /*        [8] */
      unsigned int   dis1                              :  1 ;  /*        [7] */
      unsigned int   en1                               :  1 ;  /*        [6] */
      unsigned int   clr1                              :  1 ;  /*        [5] */
      unsigned int   set1                              :  1 ;  /*        [4] */
      unsigned int   dis0                              :  1 ;  /*        [3] */
      unsigned int   en0                               :  1 ;  /*        [2] */
      unsigned int   clr0                              :  1 ;  /*        [1] */
      unsigned int   set0                              :  1 ;  /*        [0] */
   } gp0_3_out ;

#define IO_GP0_3_OUT_SET0_MASK                                           0x0001
#define IO_GP0_3_OUT_CLR0_MASK                                           0x0002
#define IO_GP0_3_OUT_EN0_MASK                                            0x0004
#define IO_GP0_3_OUT_DIS0_MASK                                           0x0008
#define IO_GP0_3_OUT_SET1_MASK                                           0x0010
#define IO_GP0_3_OUT_CLR1_MASK                                           0x0020
#define IO_GP0_3_OUT_EN1_MASK                                            0x0040
#define IO_GP0_3_OUT_DIS1_MASK                                           0x0080
#define IO_GP0_3_OUT_SET2_MASK                                           0x0100
#define IO_GP0_3_OUT_CLR2_MASK                                           0x0200
#define IO_GP0_3_OUT_EN2_MASK                                            0x0400
#define IO_GP0_3_OUT_DIS2_MASK                                           0x0800
#define IO_GP0_3_OUT_SET3_MASK                                           0x1000
#define IO_GP0_3_OUT_CLR3_MASK                                           0x2000
#define IO_GP0_3_OUT_EN3_MASK                                            0x4000
#define IO_GP0_3_OUT_DIS3_MASK                                           0x8000

   struct
   {
      unsigned int   dis7                              :  1 ;  /*       [15] */
      unsigned int   en7                               :  1 ;  /*       [14] */
      unsigned int   clr7                              :  1 ;  /*       [13] */
      unsigned int   set7                              :  1 ;  /*       [12] */
      unsigned int   dis6                              :  1 ;  /*       [11] */
      unsigned int   en6                               :  1 ;  /*       [10] */
      unsigned int   clr6                              :  1 ;  /*        [9] */
      unsigned int   set6                              :  1 ;  /*        [8] */
      unsigned int   dis5                              :  1 ;  /*        [7] */
      unsigned int   en5                               :  1 ;  /*        [6] */
      unsigned int   clr5                              :  1 ;  /*        [5] */
      unsigned int   set5                              :  1 ;  /*        [4] */
      unsigned int   dis4                              :  1 ;  /*        [3] */
      unsigned int   en4                               :  1 ;  /*        [2] */
      unsigned int   clr4                              :  1 ;  /*        [1] */
      unsigned int   set4                              :  1 ;  /*        [0] */
   } gp4_7_out ;

#define IO_GP4_7_OUT_SET4_MASK                                           0x0001
#define IO_GP4_7_OUT_CLR4_MASK                                           0x0002
#define IO_GP4_7_OUT_EN4_MASK                                            0x0004
#define IO_GP4_7_OUT_DIS4_MASK                                           0x0008
#define IO_GP4_7_OUT_SET5_MASK                                           0x0010
#define IO_GP4_7_OUT_CLR5_MASK                                           0x0020
#define IO_GP4_7_OUT_EN5_MASK                                            0x0040
#define IO_GP4_7_OUT_DIS5_MASK                                           0x0080
#define IO_GP4_7_OUT_SET6_MASK                                           0x0100
#define IO_GP4_7_OUT_CLR6_MASK                                           0x0200
#define IO_GP4_7_OUT_EN6_MASK                                            0x0400
#define IO_GP4_7_OUT_DIS6_MASK                                           0x0800
#define IO_GP4_7_OUT_SET7_MASK                                           0x1000
#define IO_GP4_7_OUT_CLR7_MASK                                           0x2000
#define IO_GP4_7_OUT_EN7_MASK                                            0x4000
#define IO_GP4_7_OUT_DIS7_MASK                                           0x8000

   struct
   {
      unsigned int   dis11                             :  1 ;  /*       [15] */
      unsigned int   en11                              :  1 ;  /*       [14] */
      unsigned int   clr11                             :  1 ;  /*       [13] */
      unsigned int   set11                             :  1 ;  /*       [12] */
      unsigned int   dis10                             :  1 ;  /*       [11] */
      unsigned int   en10                              :  1 ;  /*       [10] */
      unsigned int   clr10                             :  1 ;  /*        [9] */
      unsigned int   set10                             :  1 ;  /*        [8] */
      unsigned int   dis9                              :  1 ;  /*        [7] */
      unsigned int   en9                               :  1 ;  /*        [6] */
      unsigned int   clr9                              :  1 ;  /*        [5] */
      unsigned int   set9                              :  1 ;  /*        [4] */
      unsigned int   dis8                              :  1 ;  /*        [3] */
      unsigned int   en8                               :  1 ;  /*        [2] */
      unsigned int   clr8                              :  1 ;  /*        [1] */
      unsigned int   set8                              :  1 ;  /*        [0] */
   } gp8_11_out ;

#define IO_GP8_11_OUT_SET8_MASK                                          0x0001
#define IO_GP8_11_OUT_CLR8_MASK                                          0x0002
#define IO_GP8_11_OUT_EN8_MASK                                           0x0004
#define IO_GP8_11_OUT_DIS8_MASK                                          0x0008
#define IO_GP8_11_OUT_SET9_MASK                                          0x0010
#define IO_GP8_11_OUT_CLR9_MASK                                          0x0020
#define IO_GP8_11_OUT_EN9_MASK                                           0x0040
#define IO_GP8_11_OUT_DIS9_MASK                                          0x0080
#define IO_GP8_11_OUT_SET10_MASK                                         0x0100
#define IO_GP8_11_OUT_CLR10_MASK                                         0x0200
#define IO_GP8_11_OUT_EN10_MASK                                          0x0400
#define IO_GP8_11_OUT_DIS10_MASK                                         0x0800
#define IO_GP8_11_OUT_SET11_MASK                                         0x1000
#define IO_GP8_11_OUT_CLR11_MASK                                         0x2000
#define IO_GP8_11_OUT_EN11_MASK                                          0x4000
#define IO_GP8_11_OUT_DIS11_MASK                                         0x8000

   struct
   {
      unsigned int   dis15                             :  1 ;  /*       [15] */
      unsigned int   en15                              :  1 ;  /*       [14] */
      unsigned int   clr15                             :  1 ;  /*       [13] */
      unsigned int   set15                             :  1 ;  /*       [12] */
      unsigned int   dis14                             :  1 ;  /*       [11] */
      unsigned int   en14                              :  1 ;  /*       [10] */
      unsigned int   clr14                             :  1 ;  /*        [9] */
      unsigned int   set14                             :  1 ;  /*        [8] */
      unsigned int   dis13                             :  1 ;  /*        [7] */
      unsigned int   en13                              :  1 ;  /*        [6] */
      unsigned int   clr13                             :  1 ;  /*        [5] */
      unsigned int   set13                             :  1 ;  /*        [4] */
      unsigned int   dis12                             :  1 ;  /*        [3] */
      unsigned int   en12                              :  1 ;  /*        [2] */
      unsigned int   clr12                             :  1 ;  /*        [1] */
      unsigned int   set12                             :  1 ;  /*        [0] */
   } gp12_15_out ;

#define IO_GP12_15_OUT_SET12_MASK                                        0x0001
#define IO_GP12_15_OUT_CLR12_MASK                                        0x0002
#define IO_GP12_15_OUT_EN12_MASK                                         0x0004
#define IO_GP12_15_OUT_DIS12_MASK                                        0x0008
#define IO_GP12_15_OUT_SET13_MASK                                        0x0010
#define IO_GP12_15_OUT_CLR13_MASK                                        0x0020
#define IO_GP12_15_OUT_EN13_MASK                                         0x0040
#define IO_GP12_15_OUT_DIS13_MASK                                        0x0080
#define IO_GP12_15_OUT_SET14_MASK                                        0x0100
#define IO_GP12_15_OUT_CLR14_MASK                                        0x0200
#define IO_GP12_15_OUT_EN14_MASK                                         0x0400
#define IO_GP12_15_OUT_DIS14_MASK                                        0x0800
#define IO_GP12_15_OUT_SET15_MASK                                        0x1000
#define IO_GP12_15_OUT_CLR15_MASK                                        0x2000
#define IO_GP12_15_OUT_EN15_MASK                                         0x4000
#define IO_GP12_15_OUT_DIS15_MASK                                        0x8000

   struct
   {
      unsigned int   dis19                             :  1 ;  /*       [15] */
      unsigned int   en19                              :  1 ;  /*       [14] */
      unsigned int   clr19                             :  1 ;  /*       [13] */
      unsigned int   set19                             :  1 ;  /*       [12] */
      unsigned int   dis18                             :  1 ;  /*       [11] */
      unsigned int   en18                              :  1 ;  /*       [10] */
      unsigned int   clr18                             :  1 ;  /*        [9] */
      unsigned int   set18                             :  1 ;  /*        [8] */
      unsigned int   dis17                             :  1 ;  /*        [7] */
      unsigned int   en17                              :  1 ;  /*        [6] */
      unsigned int   clr17                             :  1 ;  /*        [5] */
      unsigned int   set17                             :  1 ;  /*        [4] */
      unsigned int   dis16                             :  1 ;  /*        [3] */
      unsigned int   en16                              :  1 ;  /*        [2] */
      unsigned int   clr16                             :  1 ;  /*        [1] */
      unsigned int   set16                             :  1 ;  /*        [0] */
   } gp16_19_out ;

#define IO_GP16_19_OUT_SET16_MASK                                        0x0001
#define IO_GP16_19_OUT_CLR16_MASK                                        0x0002
#define IO_GP16_19_OUT_EN16_MASK                                         0x0004
#define IO_GP16_19_OUT_DIS16_MASK                                        0x0008
#define IO_GP16_19_OUT_SET17_MASK                                        0x0010
#define IO_GP16_19_OUT_CLR17_MASK                                        0x0020
#define IO_GP16_19_OUT_EN17_MASK                                         0x0040
#define IO_GP16_19_OUT_DIS17_MASK                                        0x0080
#define IO_GP16_19_OUT_SET18_MASK                                        0x0100
#define IO_GP16_19_OUT_CLR18_MASK                                        0x0200
#define IO_GP16_19_OUT_EN18_MASK                                         0x0400
#define IO_GP16_19_OUT_DIS18_MASK                                        0x0800
#define IO_GP16_19_OUT_SET19_MASK                                        0x1000
#define IO_GP16_19_OUT_CLR19_MASK                                        0x2000
#define IO_GP16_19_OUT_EN19_MASK                                         0x4000
#define IO_GP16_19_OUT_DIS19_MASK                                        0x8000

   struct
   {
      unsigned int   dis23                             :  1 ;  /*       [15] */
      unsigned int   en23                              :  1 ;  /*       [14] */
      unsigned int   clr23                             :  1 ;  /*       [13] */
      unsigned int   set23                             :  1 ;  /*       [12] */
      unsigned int   dis22                             :  1 ;  /*       [11] */
      unsigned int   en22                              :  1 ;  /*       [10] */
      unsigned int   clr22                             :  1 ;  /*        [9] */
      unsigned int   set22                             :  1 ;  /*        [8] */
      unsigned int   dis21                             :  1 ;  /*        [7] */
      unsigned int   en21                              :  1 ;  /*        [6] */
      unsigned int   clr21                             :  1 ;  /*        [5] */
      unsigned int   set21                             :  1 ;  /*        [4] */
      unsigned int   dis20                             :  1 ;  /*        [3] */
      unsigned int   en20                              :  1 ;  /*        [2] */
      unsigned int   clr20                             :  1 ;  /*        [1] */
      unsigned int   set20                             :  1 ;  /*        [0] */
   } gp20_23_out ;

#define IO_GP20_23_OUT_SET20_MASK                                        0x0001
#define IO_GP20_23_OUT_CLR20_MASK                                        0x0002
#define IO_GP20_23_OUT_EN20_MASK                                         0x0004
#define IO_GP20_23_OUT_DIS20_MASK                                        0x0008
#define IO_GP20_23_OUT_SET21_MASK                                        0x0010
#define IO_GP20_23_OUT_CLR21_MASK                                        0x0020
#define IO_GP20_23_OUT_EN21_MASK                                         0x0040
#define IO_GP20_23_OUT_DIS21_MASK                                        0x0080
#define IO_GP20_23_OUT_SET22_MASK                                        0x0100
#define IO_GP20_23_OUT_CLR22_MASK                                        0x0200
#define IO_GP20_23_OUT_EN22_MASK                                         0x0400
#define IO_GP20_23_OUT_DIS22_MASK                                        0x0800
#define IO_GP20_23_OUT_SET23_MASK                                        0x1000
#define IO_GP20_23_OUT_CLR23_MASK                                        0x2000
#define IO_GP20_23_OUT_EN23_MASK                                         0x4000
#define IO_GP20_23_OUT_DIS23_MASK                                        0x8000

   struct
   {
      unsigned int   dis27                             :  1 ;  /*       [15] */
      unsigned int   en27                              :  1 ;  /*       [14] */
      unsigned int   clr27                             :  1 ;  /*       [13] */
      unsigned int   set27                             :  1 ;  /*       [12] */
      unsigned int   dis26                             :  1 ;  /*       [11] */
      unsigned int   en26                              :  1 ;  /*       [10] */
      unsigned int   clr26                             :  1 ;  /*        [9] */
      unsigned int   set26                             :  1 ;  /*        [8] */
      unsigned int   dis25                             :  1 ;  /*        [7] */
      unsigned int   en25                              :  1 ;  /*        [6] */
      unsigned int   clr25                             :  1 ;  /*        [5] */
      unsigned int   set25                             :  1 ;  /*        [4] */
      unsigned int   dis24                             :  1 ;  /*        [3] */
      unsigned int   en24                              :  1 ;  /*        [2] */
      unsigned int   clr24                             :  1 ;  /*        [1] */
      unsigned int   set24                             :  1 ;  /*        [0] */
   } gp24_27_out ;

#define IO_GP24_27_OUT_SET24_MASK                                        0x0001
#define IO_GP24_27_OUT_CLR24_MASK                                        0x0002
#define IO_GP24_27_OUT_EN24_MASK                                         0x0004
#define IO_GP24_27_OUT_DIS24_MASK                                        0x0008
#define IO_GP24_27_OUT_SET25_MASK                                        0x0010
#define IO_GP24_27_OUT_CLR25_MASK                                        0x0020
#define IO_GP24_27_OUT_EN25_MASK                                         0x0040
#define IO_GP24_27_OUT_DIS25_MASK                                        0x0080
#define IO_GP24_27_OUT_SET26_MASK                                        0x0100
#define IO_GP24_27_OUT_CLR26_MASK                                        0x0200
#define IO_GP24_27_OUT_EN26_MASK                                         0x0400
#define IO_GP24_27_OUT_DIS26_MASK                                        0x0800
#define IO_GP24_27_OUT_SET27_MASK                                        0x1000
#define IO_GP24_27_OUT_CLR27_MASK                                        0x2000
#define IO_GP24_27_OUT_EN27_MASK                                         0x4000
#define IO_GP24_27_OUT_DIS27_MASK                                        0x8000

   struct
   {
      unsigned int   __fill0                           : 12 ;  /*     [15:4] */
      unsigned int   dis28                             :  1 ;  /*        [3] */
      unsigned int   en28                              :  1 ;  /*        [2] */
      unsigned int   clr28                             :  1 ;  /*        [1] */
      unsigned int   set28                             :  1 ;  /*        [0] */
   } gp28_out ;

#define IO_GP28_OUT_SET28_MASK                                           0x0001
#define IO_GP28_OUT_CLR28_MASK                                           0x0002
#define IO_GP28_OUT_EN28_MASK                                            0x0004
#define IO_GP28_OUT_DIS28_MASK                                           0x0008

   struct
   {
      unsigned int   int7                              :  1 ;  /*       [15] */
      unsigned int   sts7                              :  1 ;  /*       [14] */
      unsigned int   int6                              :  1 ;  /*       [13] */
      unsigned int   sts6                              :  1 ;  /*       [12] */
      unsigned int   int5                              :  1 ;  /*       [11] */
      unsigned int   sts5                              :  1 ;  /*       [10] */
      unsigned int   int4                              :  1 ;  /*        [9] */
      unsigned int   sts4                              :  1 ;  /*        [8] */
      unsigned int   int3                              :  1 ;  /*        [7] */
      unsigned int   sts3                              :  1 ;  /*        [6] */
      unsigned int   int2                              :  1 ;  /*        [5] */
      unsigned int   sts2                              :  1 ;  /*        [4] */
      unsigned int   int1                              :  1 ;  /*        [3] */
      unsigned int   sts1                              :  1 ;  /*        [2] */
      unsigned int   int0                              :  1 ;  /*        [1] */
      unsigned int   sts0                              :  1 ;  /*        [0] */
   } gp0_7_sts ;

#define IO_GP0_7_STS_STS0_MASK                                           0x0001
#define IO_GP0_7_STS_INT0_MASK                                           0x0002
#define IO_GP0_7_STS_STS1_MASK                                           0x0004
#define IO_GP0_7_STS_INT1_MASK                                           0x0008
#define IO_GP0_7_STS_STS2_MASK                                           0x0010
#define IO_GP0_7_STS_INT2_MASK                                           0x0020
#define IO_GP0_7_STS_STS3_MASK                                           0x0040
#define IO_GP0_7_STS_INT3_MASK                                           0x0080
#define IO_GP0_7_STS_STS4_MASK                                           0x0100
#define IO_GP0_7_STS_INT4_MASK                                           0x0200
#define IO_GP0_7_STS_STS5_MASK                                           0x0400
#define IO_GP0_7_STS_INT5_MASK                                           0x0800
#define IO_GP0_7_STS_STS6_MASK                                           0x1000
#define IO_GP0_7_STS_INT6_MASK                                           0x2000
#define IO_GP0_7_STS_STS7_MASK                                           0x4000
#define IO_GP0_7_STS_INT7_MASK                                           0x8000

   struct
   {
      unsigned int   int15                             :  1 ;  /*       [15] */
      unsigned int   sts15                             :  1 ;  /*       [14] */
      unsigned int   int14                             :  1 ;  /*       [13] */
      unsigned int   sts14                             :  1 ;  /*       [12] */
      unsigned int   int13                             :  1 ;  /*       [11] */
      unsigned int   sts13                             :  1 ;  /*       [10] */
      unsigned int   int12                             :  1 ;  /*        [9] */
      unsigned int   sts12                             :  1 ;  /*        [8] */
      unsigned int   int11                             :  1 ;  /*        [7] */
      unsigned int   sts11                             :  1 ;  /*        [6] */
      unsigned int   int10                             :  1 ;  /*        [5] */
      unsigned int   sts10                             :  1 ;  /*        [4] */
      unsigned int   int9                              :  1 ;  /*        [3] */
      unsigned int   sts9                              :  1 ;  /*        [2] */
      unsigned int   int8                              :  1 ;  /*        [1] */
      unsigned int   sts8                              :  1 ;  /*        [0] */
   } gp8_15_sts ;

#define IO_GP8_15_STS_STS8_MASK                                          0x0001
#define IO_GP8_15_STS_INT8_MASK                                          0x0002
#define IO_GP8_15_STS_STS9_MASK                                          0x0004
#define IO_GP8_15_STS_INT9_MASK                                          0x0008
#define IO_GP8_15_STS_STS10_MASK                                         0x0010
#define IO_GP8_15_STS_INT10_MASK                                         0x0020
#define IO_GP8_15_STS_STS11_MASK                                         0x0040
#define IO_GP8_15_STS_INT11_MASK                                         0x0080
#define IO_GP8_15_STS_STS12_MASK                                         0x0100
#define IO_GP8_15_STS_INT12_MASK                                         0x0200
#define IO_GP8_15_STS_STS13_MASK                                         0x0400
#define IO_GP8_15_STS_INT13_MASK                                         0x0800
#define IO_GP8_15_STS_STS14_MASK                                         0x1000
#define IO_GP8_15_STS_INT14_MASK                                         0x2000
#define IO_GP8_15_STS_STS15_MASK                                         0x4000
#define IO_GP8_15_STS_INT15_MASK                                         0x8000

   struct
   {
      unsigned int   int23                             :  1 ;  /*       [15] */
      unsigned int   sts23                             :  1 ;  /*       [14] */
      unsigned int   int22                             :  1 ;  /*       [13] */
      unsigned int   sts22                             :  1 ;  /*       [12] */
      unsigned int   int21                             :  1 ;  /*       [11] */
      unsigned int   sts21                             :  1 ;  /*       [10] */
      unsigned int   int20                             :  1 ;  /*        [9] */
      unsigned int   sts20                             :  1 ;  /*        [8] */
      unsigned int   int19                             :  1 ;  /*        [7] */
      unsigned int   sts19                             :  1 ;  /*        [6] */
      unsigned int   int18                             :  1 ;  /*        [5] */
      unsigned int   sts18                             :  1 ;  /*        [4] */
      unsigned int   int17                             :  1 ;  /*        [3] */
      unsigned int   sts17                             :  1 ;  /*        [2] */
      unsigned int   int16                             :  1 ;  /*        [1] */
      unsigned int   sts16                             :  1 ;  /*        [0] */
   } gp16_23_sts ;

#define IO_GP16_23_STS_STS16_MASK                                        0x0001
#define IO_GP16_23_STS_INT16_MASK                                        0x0002
#define IO_GP16_23_STS_STS17_MASK                                        0x0004
#define IO_GP16_23_STS_INT17_MASK                                        0x0008
#define IO_GP16_23_STS_STS18_MASK                                        0x0010
#define IO_GP16_23_STS_INT18_MASK                                        0x0020
#define IO_GP16_23_STS_STS19_MASK                                        0x0040
#define IO_GP16_23_STS_INT19_MASK                                        0x0080
#define IO_GP16_23_STS_STS20_MASK                                        0x0100
#define IO_GP16_23_STS_INT20_MASK                                        0x0200
#define IO_GP16_23_STS_STS21_MASK                                        0x0400
#define IO_GP16_23_STS_INT21_MASK                                        0x0800
#define IO_GP16_23_STS_STS22_MASK                                        0x1000
#define IO_GP16_23_STS_INT22_MASK                                        0x2000
#define IO_GP16_23_STS_STS23_MASK                                        0x4000
#define IO_GP16_23_STS_INT23_MASK                                        0x8000

   struct
   {
      unsigned int   __fill0                           :  6 ;  /*    [15:10] */
      unsigned int   int28                             :  1 ;  /*        [9] */
      unsigned int   sts28                             :  1 ;  /*        [8] */
      unsigned int   int27                             :  1 ;  /*        [7] */
      unsigned int   sts27                             :  1 ;  /*        [6] */
      unsigned int   int26                             :  1 ;  /*        [5] */
      unsigned int   sts26                             :  1 ;  /*        [4] */
      unsigned int   int25                             :  1 ;  /*        [3] */
      unsigned int   sts25                             :  1 ;  /*        [2] */
      unsigned int   int24                             :  1 ;  /*        [1] */
      unsigned int   sts24                             :  1 ;  /*        [0] */
   } gp24_28_sts ;

#define IO_GP24_28_STS_STS24_MASK                                        0x0001
#define IO_GP24_28_STS_INT24_MASK                                        0x0002
#define IO_GP24_28_STS_STS25_MASK                                        0x0004
#define IO_GP24_28_STS_INT25_MASK                                        0x0008
#define IO_GP24_28_STS_STS26_MASK                                        0x0010
#define IO_GP24_28_STS_INT26_MASK                                        0x0020
#define IO_GP24_28_STS_STS27_MASK                                        0x0040
#define IO_GP24_28_STS_INT27_MASK                                        0x0080
#define IO_GP24_28_STS_STS28_MASK                                        0x0100
#define IO_GP24_28_STS_INT28_MASK                                        0x0200

} ;



struct __ehi_fd_tag
{
   struct
   {
      unsigned int   data_size                         :  1 ;  /*       [15] */
      unsigned int   dma_ack_act_prd2                  :  2 ;  /*    [14:13] */
      unsigned int   dma_ack_act_prd1                  :  2 ;  /*    [12:11] */
      unsigned int   dma_ack_pol                       :  1 ;  /*       [10] */
      unsigned int   dma_req_pol                       :  1 ;  /*        [9] */
      unsigned int   dma_mode                          :  1 ;  /*        [8] */
      unsigned int   dma_rw                            :  1 ;  /*        [7] */
      unsigned int   __fill0                           :  3 ;  /*      [6:4] */
      unsigned int   mmp_rw_pol                        :  1 ;  /*        [3] */
      unsigned int   mmp_wait_pol                      :  1 ;  /*        [2] */
      unsigned int   mmp_cs_pol                        :  1 ;  /*        [1] */
      unsigned int   mmp_ram_wnd                       :  1 ;  /*        [0] */
   } cfg ;

#define EHI_CFG_MMP_RAM_WND_LOC256                             1
#define EHI_CFG_MMP_RAM_WND_LOC16                              0

#define EHI_CFG_MMP_CS_POL_ACT_HIGH                            1
#define EHI_CFG_MMP_CS_POL_ACT_LOW                             0

#define EHI_CFG_MMP_WAIT_POL_ACT_HIGH                          1
#define EHI_CFG_MMP_WAIT_POL_ACT_LOW                           0

#define EHI_CFG_MMP_RW_POL_ACT_HIGH                            1
#define EHI_CFG_MMP_RW_POL_ACT_LOW                             0

#define EHI_CFG_DMA_RW_READ                                    1
#define EHI_CFG_DMA_RW_WRITE                                   0

#define EHI_CFG_DMA_MODE_MASTER                                1
#define EHI_CFG_DMA_MODE_SLAVE                                 0

#define EHI_CFG_DMA_REQ_POL_ACT_HIGH                           1
#define EHI_CFG_DMA_REQ_POL_ACT_LOW                            0

#define EHI_CFG_DMA_ACK_POL_ACT_HIGH                           1
#define EHI_CFG_DMA_ACK_POL_ACT_LOW                            0

#define EHI_CFG_DATA_SIZE_D16                                  1
#define EHI_CFG_DATA_SIZE_D32                                  0

#define EHI_CFG_MMP_RAM_WND_MASK                                         0x0001
#define EHI_CFG_MMP_CS_POL_MASK                                          0x0002
#define EHI_CFG_MMP_WAIT_POL_MASK                                        0x0004
#define EHI_CFG_MMP_RW_POL_MASK                                          0x0008
#define EHI_CFG_DMA_RW_MASK                                              0x0080
#define EHI_CFG_DMA_MODE_MASK                                            0x0100
#define EHI_CFG_DMA_REQ_POL_MASK                                         0x0200
#define EHI_CFG_DMA_ACK_POL_MASK                                         0x0400
#define EHI_CFG_DMA_ACK_ACT_PRD1_MASK                                    0x1800
#define EHI_CFG_DMA_ACK_ACT_PRD2_MASK                                    0x6000
#define EHI_CFG_DATA_SIZE_MASK                                           0x8000

   struct
   {
      unsigned int   __fill0                           :  6 ;  /*    [15:10] */
      unsigned int   dma_act                           :  1 ;  /*        [9] */
      unsigned int   mmp_act                           :  1 ;  /*        [8] */
      unsigned int   __fill1                           :  4 ;  /*      [7:4] */
      unsigned int   dma_dis                           :  1 ;  /*        [3] */
      unsigned int   dma_en                            :  1 ;  /*        [2] */
      unsigned int   mmp_dis                           :  1 ;  /*        [1] */
      unsigned int   mmp_en                            :  1 ;  /*        [0] */
   } ctrl_sts ;

#define EHI_CTRL_STS_MMP_EN_MASK                                         0x0001
#define EHI_CTRL_STS_MMP_DIS_MASK                                        0x0002
#define EHI_CTRL_STS_DMA_EN_MASK                                         0x0004
#define EHI_CTRL_STS_DMA_DIS_MASK                                        0x0008
#define EHI_CTRL_STS_MMP_ACT_MASK                                        0x0100
#define EHI_CTRL_STS_DMA_ACT_MASK                                        0x0200

   struct
   {
      unsigned int   __fill0                           :  8 ;  /*     [15:8] */
      unsigned int   mmp_ram_phy                       :  8 ;  /*      [7:0] */
   } mmp_ram_phy ;

#define EHI_MMP_RAM_PHY_MMP_RAM_PHY_MASK                                 0x00FF

   struct
   {
      unsigned int   rw                                :  1 ;  /*       [15] */
      unsigned int   __fill0                           :  4 ;  /*    [14:11] */
      unsigned int   adr                               : 11 ;  /*     [10:0] */
   } mmp_hist ;

#define EHI_MMP_HIST_RW_RD                                     1
#define EHI_MMP_HIST_RW_WR                                     0

#define EHI_MMP_HIST_ADR_MASK                                            0x07FF
#define EHI_MMP_HIST_RW_MASK                                             0x8000

   struct
   {
      unsigned int   int_cfg                           :  1 ;  /*       [15] */
      unsigned int   wrap_en                           :  1 ;  /*       [14] */
      unsigned int   __fill0                           :  3 ;  /*    [13:11] */
      unsigned int   adr                               : 11 ;  /*     [10:0] */
   } dma_cfg ;

#define EHI_DMA_CFG_INT_CFG_HALF                               1
#define EHI_DMA_CFG_INT_CFG_COMPLETE                           0

#define EHI_DMA_CFG_ADR_MASK                                             0x07FF
#define EHI_DMA_CFG_WRAP_EN_MASK                                         0x4000
#define EHI_DMA_CFG_INT_CFG_MASK                                         0x8000

   struct
   {
      unsigned int   req                               :  1 ;  /*       [15] */
      unsigned int   rst                               :  1 ;  /*       [14] */
      unsigned int   stall_dis                         :  1 ;  /*       [13] */
      unsigned int   stall_en                          :  1 ;  /*       [12] */
      unsigned int   __fill0                           :  4 ;  /*     [11:8] */
      unsigned int   size                              :  8 ;  /*      [7:0] */
   } dma_ctrl ;

#define EHI_DMA_CTRL_SIZE_MASK                                           0x00FF
#define EHI_DMA_CTRL_STALL_EN_MASK                                       0x1000
#define EHI_DMA_CTRL_STALL_DIS_MASK                                      0x2000
#define EHI_DMA_CTRL_RST_MASK                                            0x4000
#define EHI_DMA_CTRL_REQ_MASK                                            0x8000

   struct
   {
      unsigned int   invalid                           :  1 ;  /*       [15] */
      unsigned int   __fill0                           :  7 ;  /*     [14:8] */
      unsigned int   value                             :  8 ;  /*      [7:0] */
   } dma_xfr ;

#define EHI_DMA_XFR_VALUE_MASK                                           0x00FF
#define EHI_DMA_XFR_INVALID_MASK                                         0x8000

   struct
   {
      unsigned int   dma_finish                        :  1 ;  /*       [15] */
      unsigned int   dma_rdy                           :  1 ;  /*       [14] */
      unsigned int   __fill0                           : 13 ;  /*     [13:1] */
      unsigned int   mmp_acc                           :  1 ;  /*        [0] */
   } int_sts ;

#define EHI_INT_STS_MMP_ACC_MASK                                         0x0001
#define EHI_INT_STS_DMA_RDY_MASK                                         0x4000
#define EHI_INT_STS_DMA_FINISH_MASK                                      0x8000

   struct
   {
      unsigned int   dma_done                          :  1 ;  /*       [15] */
      unsigned int   dma_rdy                           :  1 ;  /*       [14] */
      unsigned int   __fill0                           : 13 ;  /*     [13:1] */
      unsigned int   mmp_acc                           :  1 ;  /*        [0] */
   } int_en ;

#define EHI_INT_EN_MMP_ACC_MASK                                          0x0001
#define EHI_INT_EN_DMA_RDY_MASK                                          0x4000
#define EHI_INT_EN_DMA_DONE_MASK                                         0x8000

   struct
   {
      unsigned int   dma_done                          :  1 ;  /*       [15] */
      unsigned int   dma_rdy                           :  1 ;  /*       [14] */
      unsigned int   __fill0                           : 13 ;  /*     [13:1] */
      unsigned int   mmp_acc                           :  1 ;  /*        [0] */
   } int_dis ;

#define EHI_INT_DIS_MMP_ACC_MASK                                         0x0001
#define EHI_INT_DIS_DMA_RDY_MASK                                         0x4000
#define EHI_INT_DIS_DMA_DONE_MASK                                        0x8000

   struct
   {
      unsigned int   dma_done                          :  1 ;  /*       [15] */
      unsigned int   __fill0                           : 14 ;  /*     [14:1] */
      unsigned int   mmp_acc                           :  1 ;  /*        [0] */
   } int_clr ;

#define EHI_INT_CLR_MMP_ACC_MASK                                         0x0001
#define EHI_INT_CLR_DMA_DONE_MASK                                        0x8000

} ;



struct __adc_fd_tag
{
   struct
   {
      unsigned int   rdy                               :  1 ;  /*       [15] */
      unsigned int   ovfl                              :  1 ;  /*       [14] */
      unsigned int   wrap                              :  1 ;  /*       [13] */
      unsigned int   __fill0                           :  1 ;  /*       [12] */
      unsigned int   data                              : 12 ;  /*     [11:0] */
   } sts ;

#define ADC_STS_DATA_MASK                                                0x0FFF
#define ADC_STS_WRAP_MASK                                                0x2000
#define ADC_STS_OVFL_MASK                                                0x4000
#define ADC_STS_RDY_MASK                                                 0x8000

   unsigned int __fill0 ;                                      /*       FFC5 */

   struct
   {
      unsigned int   __fill0                           :  8 ;  /*     [15:8] */
      unsigned int   temp_en                           :  1 ;  /*        [7] */
      unsigned int   vsens_en                          :  1 ;  /*        [6] */
      unsigned int   temp_sel                          :  1 ;  /*        [5] */
      unsigned int   vsens_sel                         :  1 ;  /*        [4] */
      unsigned int   ip_sel                            :  4 ;  /*      [3:0] */
   } cfg ;

#define ADC_CFG_IP_SEL_IP13                                    13
#define ADC_CFG_IP_SEL_IP12                                    12
#define ADC_CFG_IP_SEL_IP10                                    10
#define ADC_CFG_IP_SEL_IP9                                     9
#define ADC_CFG_IP_SEL_IP8                                     8
#define ADC_CFG_IP_SEL_IP6                                     6
#define ADC_CFG_IP_SEL_IP5                                     5
#define ADC_CFG_IP_SEL_IP4                                     4
#define ADC_CFG_IP_SEL_IP3                                     3
#define ADC_CFG_IP_SEL_IP2                                     2
#define ADC_CFG_IP_SEL_IP1                                     1
#define ADC_CFG_IP_SEL_IP0                                     0

#define ADC_CFG_IP_SEL_MASK                                              0x000F
#define ADC_CFG_VSENS_SEL_MASK                                           0x0010
#define ADC_CFG_TEMP_SEL_MASK                                            0x0020
#define ADC_CFG_VSENS_EN_MASK                                            0x0040
#define ADC_CFG_TEMP_EN_MASK                                             0x0080

   struct
   {
      unsigned int   __fill0                           : 13 ;  /*     [15:3] */
      unsigned int   int_clr                           :  1 ;  /*        [2] */
      unsigned int   int_dis                           :  1 ;  /*        [1] */
      unsigned int   int_en                            :  1 ;  /*        [0] */
   } ctrl ;

#define ADC_CTRL_INT_EN_MASK                                             0x0001
#define ADC_CTRL_INT_DIS_MASK                                            0x0002
#define ADC_CTRL_INT_CLR_MASK                                            0x0004

} ;



struct __flash_fd_tag
{
   struct
   {
      unsigned int   last_wr                           :  1 ;  /*       [15] */
      unsigned int   __fill0                           :  6 ;  /*     [14:9] */
      unsigned int   period                            :  9 ;  /*      [8:0] */
   } prg_cfg ;

#define FLASH_PRG_CFG_PERIOD_MASK                                        0x01FF
#define FLASH_PRG_CFG_LAST_WR_MASK                                       0x8000

   struct
   {
      unsigned int   safe                              :  8 ;  /*     [15:8] */
      unsigned int   cmd                               :  8 ;  /*      [7:0] */
   } prg_ctrl ;

#define FLASH_PRG_CTRL_CMD_MASK                                          0x00FF
#define FLASH_PRG_CTRL_SAFE_MASK                                         0xFF00

   struct
   {
      unsigned int   bsy                               :  1 ;  /*       [15] */
      unsigned int   adr                               : 15 ;  /*     [14:0] */
   } prg_adr ;

#define FLASH_PRG_ADR_ADR_MASK                                           0x7FFF
#define FLASH_PRG_ADR_BSY_MASK                                           0x8000

   struct
   {
      unsigned int   prg_data                          : 16 ;  /*     [15:0] */
   } prg_data ;

#define FLASH_PRG_DATA_PRG_DATA_MASK                                     0xFFFF

   struct
   {
      unsigned int   bsy                               :  1 ;  /*       [15] */
      unsigned int   __fill0                           :  9 ;  /*     [14:6] */
      unsigned int   adr                               :  6 ;  /*      [5:0] */
   } inf_rd_adr ;

#define FLASH_INF_RD_ADR_ADR_MASK                                        0x003F
#define FLASH_INF_RD_ADR_BSY_MASK                                        0x8000

   struct
   {
      unsigned int   inf_rd_data                       : 16 ;  /*     [15:0] */
   } inf_rd_data ;

#define FLASH_INF_RD_DATA_INF_RD_DATA_MASK                               0xFFFF

   struct
   {
      unsigned int   s7wp                              :  1 ;  /*       [15] */
      unsigned int   s6wp                              :  1 ;  /*       [14] */
      unsigned int   s5wp                              :  1 ;  /*       [13] */
      unsigned int   s4wp                              :  1 ;  /*       [12] */
      unsigned int   s3wp                              :  1 ;  /*       [11] */
      unsigned int   s2wp                              :  1 ;  /*       [10] */
      unsigned int   s1wp                              :  1 ;  /*        [9] */
      unsigned int   s0wp                              :  1 ;  /*        [8] */
      unsigned int   __fill0                           :  7 ;  /*      [7:1] */
      unsigned int   ifwp                              :  1 ;  /*        [0] */
   } sect_wr_prot ;

#define FLASH_SECT_WR_PROT_IFWP_MASK                                     0x0001
#define FLASH_SECT_WR_PROT_S0WP_MASK                                     0x0100
#define FLASH_SECT_WR_PROT_S1WP_MASK                                     0x0200
#define FLASH_SECT_WR_PROT_S2WP_MASK                                     0x0400
#define FLASH_SECT_WR_PROT_S3WP_MASK                                     0x0800
#define FLASH_SECT_WR_PROT_S4WP_MASK                                     0x1000
#define FLASH_SECT_WR_PROT_S5WP_MASK                                     0x2000
#define FLASH_SECT_WR_PROT_S6WP_MASK                                     0x4000
#define FLASH_SECT_WR_PROT_S7WP_MASK                                     0x8000

   struct
   {
      unsigned int   s7rp                              :  1 ;  /*       [15] */
      unsigned int   s6rp                              :  1 ;  /*       [14] */
      unsigned int   s5rp                              :  1 ;  /*       [13] */
      unsigned int   s4rp                              :  1 ;  /*       [12] */
      unsigned int   s3rp                              :  1 ;  /*       [11] */
      unsigned int   s2rp                              :  1 ;  /*       [10] */
      unsigned int   s1rp                              :  1 ;  /*        [9] */
      unsigned int   s0rp                              :  1 ;  /*        [8] */
      unsigned int   __fill0                           :  7 ;  /*      [7:1] */
      unsigned int   ifrp                              :  1 ;  /*        [0] */
   } sect_rd_prot ;

#define FLASH_SECT_RD_PROT_IFRP_MASK                                     0x0001
#define FLASH_SECT_RD_PROT_S0RP_MASK                                     0x0100
#define FLASH_SECT_RD_PROT_S1RP_MASK                                     0x0200
#define FLASH_SECT_RD_PROT_S2RP_MASK                                     0x0400
#define FLASH_SECT_RD_PROT_S3RP_MASK                                     0x0800
#define FLASH_SECT_RD_PROT_S4RP_MASK                                     0x1000
#define FLASH_SECT_RD_PROT_S5RP_MASK                                     0x2000
#define FLASH_SECT_RD_PROT_S6RP_MASK                                     0x4000
#define FLASH_SECT_RD_PROT_S7RP_MASK                                     0x8000

} ;



struct __rg_tag
{
   __duart_rg_t         duart ;                                /*  FEA0-FEB3 */
   __dusart_rg_t        dusart ;                               /*  FEB4-FF1C */
   __tim_rg_t           tim ;                                  /*  FF1D-FF40 */
   __cache_rg_t         cache ;                                /*  FF41-FF42 */
   __mmu_rg_t           mmu ;                                  /*  FF43-FF69 */
   __ssm_rg_t           ssm ;                                  /*  FF6A-FF79 */
   __emi_rg_t           emi ;                                  /*  FF7A-FF81 */
   __intact_rg_t        intact ;                               /*  FF82-FF9A */
   __port_rg_t          port ;                                 /*  FF9B-FF9F */
   __io_rg_t            io ;                                   /*  FFA0-FFB8 */
   __ehi_rg_t           ehi ;                                  /*  FFB9-FFC3 */
   __adc_rg_t           adc ;                                  /*  FFC4-FFC7 */
   __flash_rg_t         flash ;                                /*  FFC8-FFCF */
} ;

extern volatile __rg_t rg ;

struct __fd_tag
{
   __duart_fd_t         duart ;                                /*  FEA0-FEB3 */
   __dusart_fd_t        dusart ;                               /*  FEB4-FF1C */
   __tim_fd_t           tim ;                                  /*  FF1D-FF40 */
   __cache_fd_t         cache ;                                /*  FF41-FF42 */
   __mmu_fd_t           mmu ;                                  /*  FF43-FF69 */
   __ssm_fd_t           ssm ;                                  /*  FF6A-FF79 */
   __emi_fd_t           emi ;                                  /*  FF7A-FF81 */
   __intact_fd_t        intact ;                               /*  FF82-FF9A */
   __port_fd_t          port ;                                 /*  FF9B-FF9F */
   __io_fd_t            io ;                                   /*  FFA0-FFB8 */
   __ehi_fd_t           ehi ;                                  /*  FFB9-FFC3 */
   __adc_fd_t           adc ;                                  /*  FFC4-FFC7 */
   __flash_fd_t         flash ;                                /*  FFC8-FFCF */
} ;

extern volatile __fd_t fd ;

#endif

/* end of ecog1.h */
