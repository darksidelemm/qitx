function phase_generator_ad9834( phasearr )
%PHASEGEN Generate a bpsk phase reversal function for the QITX Board
%   Generates a set of command words which produce a shaped phase reversal.
%   Input arguments must be between 0 and 180.

clc
disp('void bpsk_phase_switch(){')
disp('    bpsk_phase_state++;')
disp('    PORTC |= _BV(7);')
disp('    if(bpsk_phase_state&1){')
for k = 1:length(phasearr)
    phase = round(2047*phasearr(k)/180);
    disp(sprintf('        AD9834_SendWord(0xC%03X);',phase))
end

disp('    }else{')

for k = length(phasearr):-1:1
    phase = round(2047*phasearr(k)/180);
    disp(sprintf('        AD9834_SendWord(0xC%03X);',phase))
end

disp('    }')
disp('    PORTC &= ~_BV(7);')
disp('}')

end

