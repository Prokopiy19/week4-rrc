rm -r src/*
rm -r build/*
mkdir src
mkdir build
asn1c lte-rrc-15.6.0.asn1 -D src -pdu=UL-CCCH-Message -pdu=DL-CCCH-Message -pdu=UL-DCCH-Message -no-gen-OER -fcompound-names -no-gen-example -fno-include-deps
cd src
clang -g -I. -c *.c -DASN_DISABLE_OER_SUPPORT
cd ..
clang++ -g -Isrc -I. -c enb_main.cpp enb_rrc.cpp -lsctp -DASN_DISABLE_OER_SUPPORT
clang++ -g -o build/enb enb_main.o enb_rrc.o src/*.o -lsctp -DASN_DISABLE_OER_SUPPORT

clang++ -g -Isrc -I. -c ue_main.cpp ue_rrc.cpp -lsctp -DASN_DISABLE_OER_SUPPORT
clang++ -g -o build/ue ue_main.o ue_rrc.o src/*.o -lsctp -DASN_DISABLE_OER_SUPPORT

# ./build/enb
#./build/ue
