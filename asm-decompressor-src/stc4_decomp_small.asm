;~ STC4 simple tile compressor four (based on STC0) - this is a smaller version, saving some ROM but (slightly) slower
;~   [by sverx, 23/12/2024]
;~
;~ uses 4 bytes RAM buffer
;~
;~ one byte holds information on how to create a group of 4 bytes, providing two bits for each:
;~
;~     D7 D6          D5 D4            D3 D2          D1 D0
;~
;~    1st byte       2nd byte         3rd byte       4th byte
;~
;~ 0b10 -> byte has value 0x00
;~ 0b11 -> byte has value 0xFF
;~ 0b01 -> byte is a different value, uncompressed byte follows
;~ 0b00 -> repeat last uncompressed value from same group of 4 bytes
;~
;~
;~ note: if D7,D6 are both zero it means this whole group of 4 bytes is based on the previous group so:
;~
;~       if D5 = 0 then we need to repeat the previous group again, up to 31 times (counter is value in D4-D0) or EOD if counter is 0
;~
;~       else if D5 = 1 then we need to use the last group of 4 bytes replacing each value that has a 1 in D3-D0 with a raw value that follows (up to 3)
;~          [ and if D4 is 1 then it means bytes from the previous group of 4 bytes needs to be complemented, so for instance a value of $02 becomes $FD and viceversa ]
;~
;~  IN:
;~      HL (data source address)
;~      DE (destination in VRAM w/flags)
;~  clobbers:
;~      AF,BC,DE,HL,IYL
;~


; comment the following line to create a faster but VRAM unsafe version of this algorithm
.define VRAM_SAFE

.define VDP_CTRL_PORT    $bf
.define VDP_DATA_PORT    $be

.ramsection "stc4_buffer" slot 3 free
  stc4_buffer dsb 4                   ; the 4 bytes buffer for the decompression
.ends

.section "stc4_decompress" free
stc4_decompress:
  ld c,VDP_CTRL_PORT                  ; set VRAM destination address
  di
  out (c),e
  out (c),d
  ei

_stc4_decompress_outer_loop:
  ld a,(hl)
  cp $20                              ; if value less than $20 it's a rerun or an end-of-data marker
  jr c,_reruns_or_leave

  ld b,4
  ld de,stc4_buffer

_stc4_decompress_inner_loop:
  rla
  jr c,_compressed_00_or_FF           ; if 1X found, write $00 or $FF

  rla
  jr nc,_same_or_diff                 ; if 0b00 found it's same or diff

  ld c,a                              ; preserve A
  inc hl
  ld a,(hl)                           ; load uncompressed byte

_stc4_write_byte:
  out (VDP_DATA_PORT),a               ; write byte to VRAM
  ld (de),a                           ; and to buffer too
  inc de                              ; advance buffer pointer
  ld a,c                              ; restore A
  djnz _stc4_decompress_inner_loop    ; we've got more to process in this byte

  inc hl                              ; move to next byte
  jp _stc4_decompress_outer_loop
  
_compressed_00_or_FF:
  rla
  ld c,a                              ; preserve A
  sbc a                               ; this turns the CF into $00 or $FF
  jp _stc4_write_byte

_same_or_diff:
  bit 2,b
  jr nz,_diff                         ; if byte is $00nnnnnn then it means it's a diff, not a same byte in the group

_same_byte:
  ld c,a                              ; preserve A
  ld a,(hl)                           ; we won't [inc hl] here because we're loading the same value as before so we are already on that
  jp _stc4_write_byte

; ************************************
_diff:
  rla                                 ; skip D5
  ld c,a                              ; save D4 in C's MSB
  rla                                 ; skip D4

_diff_loop:
  rla
  ld iyl,a                            ; preserve A
  jr c,_raw_value_follows             ; when 1 a data byte will follow

  ld a,(de)
  bit 7,c
  jr z,_write_diff_byte

  cpl                                 ; invert data if D4 was set
  ld (de),a                           ; and save it back into the buffer

_write_diff_byte:
  out (VDP_DATA_PORT),a               ; write byte from buffer to VRAM
  ld a,iyl                            ; restore A
  inc de                              ; advance buffer pointer
  djnz _diff_loop                     ; until we're done with the 4 bits

  inc hl                              ; move to next byte
  jp _stc4_decompress_outer_loop      ; loop over

_raw_value_follows:
  inc hl
  ld a,(hl)
  ld (de),a                           ; save it into the buffer
  jp _write_diff_byte

; ************************************
_reruns_or_leave:
  and $1F
  ret z                               ; if value is zero, the EOD marker has been found, so leave

  ld b,a                              ; save reruns counter in B

_transfer_whole_buffer_B_times:
  ld de,stc4_buffer

  ld a,(de)                           ; 7
  out (VDP_DATA_PORT),a               ; 11
.ifdef VRAM_SAFE
  nop                                 ; 4
.endif
  inc de                              ; 6  = 28

  ld a,(de)                           ; 7
  out (VDP_DATA_PORT),a               ; 11
.ifdef VRAM_SAFE
  nop                                 ; 4
.endif
  inc de                              ; 6  = 28

  ld a,(de)                           ; 7
  out (VDP_DATA_PORT),a               ; 11
.ifdef VRAM_SAFE
  nop                                 ; 4
.endif
  inc de                              ; 6  = 28

  ld a,(de)                           ; 7
  out (VDP_DATA_PORT),a               ; 11
  djnz _transfer_whole_buffer_B_times ; 13 = 31

  inc hl                              ; move to next byte
  jp _stc4_decompress_outer_loop      ; loop over
.ends

.undef VDP_CTRL_PORT
.undef VDP_DATA_PORT

