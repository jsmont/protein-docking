/usr/bin/time ./ftdock -static 2pka.parsed -mobile 5pti.parsed > output.test1
/usr/bin/time ./ftdock -static 1hba.parsed -mobile 5pti.parsed > output.test2
/usr/bin/time ./ftdock -static 4hhb.parsed -mobile 5pti.parsed > output.test3
/usr/bin/time ./ftdock -static 1ACB_rec.parsed -mobile 1ACB_lig.parsed > output.test4


diff output.test1 test1.output
diff output.test2 test2.output
diff output.test3 test3.output
diff output.test4 test4.output
