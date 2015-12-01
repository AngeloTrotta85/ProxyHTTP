set terminal png
set output 'percentage.png'
set yrange [0:1]
plot 'test1_caba' w l, 'test2_caba' w l, 'test3_caba' w l, 'test1_caba_fixed' w l, 'test2_caba_fixed' w l, 'test3_caba_fixed' w l, 'test1_sbs' w l
