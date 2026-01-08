;; Example program for testing W65C02 with 6502-Teensy bridge
;; by DDRCode
;;
;; The code performs 16-bit add operation.
;; Consider that z = x+y, then
;; x is stored in $10 (hi-byte), $11 (lo-byte)
;; y is stored in $12 (hi-byte), $13 (lo-byte)
;; z is stored in $14 (hi-byte), $15 (lo-byte)
;; In the program   x=$05ff
;;                  y=$da21
;; so the result is z=$e020
;;
;; At the end, the program compares the addition with expected values.
;; Registry Y should be 0 in case of correct result and 1 otherwise.
;; Registry A should contain hi-byte of the result and X - the low-byte.
;;
;; Additionally the program contains IRQ/NMI handling routine
;; (that does nothing), but the code should never be called in
;; properly configured bridge. If you'd see it's being executed then
;; most likely your NMI or IRQ pins aren't connected correctly.
;;
;; Assembler: ACME
;; Assembling command: acme -f plain -o test.p --cpu w65c02 test.asm

; start on addres $200 (right after page Zero and the stack)
* = $0200

setup:
        ; set the interrupt vectors to $0300
        LDA #$03
        STA $ffff
        STA $fffb
        STZ $fffe
        STZ $fffa

        ; correctness indicator (1=error)
        LDY #1

        ; store $ff05 in addr $10-$11
        LDA #$05
        STA $10
        LDA #$ff
        STA $11

        ; store $da21 in addr $12-$13
        LDA #$da
        STA $12
        LDA #$21
        STA $13

add:
        ; add lo-byte and store the result in X and addr $15
        CLC
        LDA $11
        ADC $13
        STA $15
        TAX

        ; add hi-bytes (with carry) and store the result in $14
        LDA $10
        ADC $12
        STA $14

test:
        ; check the result and set Y to 0 if correct
        CMP #$e0
        BNE end
        CPX #$20
        BNE end
        LDY #0

end:
        ; end the program
        BRK         ; the C++/Rust runner by default is configured to stop
                    ; on BRK, so the program should end here, buf if not...
irqloop:
        WAI         ; ..it sends the CPU to sleep until interrupt
        BRA irqloop ; in an infinite loop fasion

; interrupt handling
; (although interrupts shouldn't happen in this program)
* = $0300
        NOP         ; do nothing
        RTI         ; and return to the program


