                  MODULE   irq
                  .ALL
;
; ============================================================================ 
;                                                                              
; Copyright (C) Olsonet Communications Corporation, 2002, 2003                 
;                                                                              
; Permission is hereby granted, free of charge, to any person obtaining a copy 
; of this software and associated documentation files (the "Software"), to     
; deal in the Software without restriction, including without limitation the   
; rights to use, copy, modify, merge, publish, distribute, sublicense, and/or  
; sell copies of the Software, and to permit persons to whom the Software is   
; furnished to do so, subject to the following conditions:                     
;                                                                              
; The above copyright notice and this permission notice shall be included in   
; all copies or substantial portions of the Software.                          
;                                                                              
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR   
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,     
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE  
; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER       
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING      
; FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
; IN THE SOFTWARE.                                                             
;                                                                              
; ============================================================================ 

;
; Interrupt vectors are sign-extended 24 bit absolute branch addresses. For
; example a vector containing H'1234 will start executing at H'1234 and a
; vector containing H'8765 will start executing at H'ff8765. All unused
; interrupts are handled by the default code that is below.
;

                  ORG      H'0
                  bra      $?cstart_code

                  ORG      H'4

;;$exception_debug_vector:
                  DC $zzz_minimal_handler

;;$interrupt_watchdog_vector:
                  DC $zzz_minimal_handler

;;$exception_address_error_vector:
                  DC $zzz_address_error
;
; Peripheral Interrupt vectors.
;
                  ORG      H'8

;;$exception_tim_vector:
                  DC       $zzz_minimal_handler

;;$exception_intact_vector:
                  DC       $zzz_minimal_handler

;;$exception_usart_a_vector:
                  DC       $zzz_minimal_handler

;;$exception_usart_b_vector:
                  DC       $zzz_minimal_handler

;;$exception_uart_a_vector:
                  DC       $uart_a_int??

;;$exception_uart_b_vector:
                  DC       $uart_b_int??

;;$interrupt_tim_rtc_underflow_vector:
                  DC       $timer_int??

;;$interrupt_tim_gpt_1_underflow_vector:
                  DC       $zzz_minimal_handler

;;$interrupt_tim_gpt_2_underflow_vector:
                  DC       $zzz_minimal_handler

;;$interrupt_tim_gpt_1_count_compare_match_vector:
                  DC       $zzz_minimal_handler

;;$interrupt_tim_gpt_2_count_compare_match_vector:
                  DC       $zzz_minimal_handler

;;$interrupt_tim_cgt_1_underflow_vector:
                  DC       $zzz_minimal_handler

;;$interrupt_tim_cgt_2_underflow_vector:
                  DC       $zzz_minimal_handler

;;$interrupt_tim_cgt_1_count_transition_match_vector:
                  DC       $zzz_minimal_handler

;;$interrupt_tim_cgt_2_count_transition_match_vector:
                  DC       $zzz_minimal_handler

;;$interrupt_tim_cpt_overflow_vector:
                  DC       $zzz_minimal_handler

;;$interrupt_tim_cpt_capture_1_vector:
                  DC       $zzz_minimal_handler

;;$interrupt_tim_cpt_capture_2_vector:
                  DC       $zzz_minimal_handler

;;$interrupt_tim_cpt_capture_3_vector:
                  DC       $zzz_minimal_handler

;;$interrupt_tim_cpt_capture_4_vector:
                  DC       $zzz_minimal_handler

;;$interrupt_tim_cpt_capture_5_vector:
                  DC       $zzz_minimal_handler

;;$interrupt_tim_cpt_capture_6_vector:
                  DC       $zzz_minimal_handler

;;$interrupt_tim_lit_undeflow_vector:
                  DC       $zzz_minimal_handler

;;$interrupt_intact_cpu_channel_rx_ready_vector:
                  DC       $zzz_minimal_handler

;;$interrupt_intact_cpu_channel_tx_ready_vector:
                  DC       $zzz_minimal_handler

;;$interrupt_intact_dma_channel_ready_vector:
                  DC       $zzz_minimal_handler

;;$interrupt_intact_dma_channel_complete_vector:
                  DC       $zzz_minimal_handler

;;$interrupt_intact_up_clock_inactive_vector:
                  DC       $zzz_minimal_handler

;;$interrupt_intact_up_rx_data_message_purge_vector:
                  DC       $zzz_minimal_handler

;;$interrupt_intact_down_clock_inactive_vector:
                  DC       $zzz_minimal_handler

;;$interrupt_intact_down_rx_data_message_purge_vector:
                  DC       $zzz_minimal_handler

;;$interrupt_usart_a_rx_vector:
                  DC       $zzz_minimal_handler

;;$interrupt_usart_a_tx_vector:
                  DC       $zzz_minimal_handler

;;$interrupt_usart_b_rx_vector:
                  DC       $zzz_minimal_handler

;;$interrupt_usart_b_tx_vector:
                  DC       $zzz_minimal_handler

;;$interrupt_sci_tx_data_complete_vector:
                  DC       $zzz_minimal_handler

;;$interrupt_sci_tx_error_detected_vector:
                  DC       $zzz_minimal_handler

;;$interrupt_sci_general:
                  DC       $zzz_minimal_handler

;;$interrupt_ifr_tx_data_complete_vector:
                  DC       $zzz_minimal_handler

;;$interrupt_ifr_rx_data_complete_vector:
                  DC       $zzz_minimal_handler

;;$interrupt_ifr_rx_error_vector:
                  DC       $zzz_minimal_handler

;;$interrupt_ifr_frame_complete_vector:
                  DC       $zzz_minimal_handler

;;$interrupt_uart_a_tx_write_ready_vector:
                  DC       $uart_a_int??

;;$interrupt_uart_a_rx_read_ready_vector:
                  DC       $uart_a_int??

;;$interrupt_uart_b_tx_write_ready_vector:
                  DC       $uart_b_int??

;;$interrupt_uart_b_rx_read_ready:
                  DC       $uart_b_int??

;;$interrupt_ehi_vector:
                  DC       $zzz_minimal_handler

;;$interrupt_gpio_any_vector:
;                 DC       $zzz_minimal_handler
; For those devices that receive interrupts via GPIO (e.g., Ethernet, CHIPCON)
                  DC       $gpio_int??

;;$interrupt_adc_ready_vector:
                  DC       $zzz_minimal_handler

                  ENDMOD
