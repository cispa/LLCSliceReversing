all: spy
clean:
	rm -rf spy openssl/libcrypto.so
openssl/crypto/aes/aes_core.c: openssl
openssl:
	git clone --depth 1 https://github.com/openssl/openssl.git openssl
openssl/libcrypto.so: openssl/crypto/aes/aes_core.c
	sed -i 's/static const u32 Te0\[256\].*/#define Te0 (Te0_real+1)\nstatic const u32 Te0_real[512] __attribute__((aligned(4096))) = { 0,/g' openssl/crypto/aes/aes_core.c
	sed -i 's/static const u32 Te1\[256\].*/#define Te1 (Te1_real+1)\nstatic const u32 Te1_real[512] __attribute__((aligned(4096))) = { 0,/g' openssl/crypto/aes/aes_core.c
	sed -i 's/static const u32 Te2\[256\].*/#define Te2 (Te2_real+1)\nstatic const u32 Te2_real[512] __attribute__((aligned(4096))) = { 0,/g' openssl/crypto/aes/aes_core.c
	sed -i 's/static const u32 Te3\[256\].*/#define Te3 (Te3_real+1)\nstatic const u32 Te3_real[512] __attribute__((aligned(4096))) = { 0,/g' openssl/crypto/aes/aes_core.c
	cd openssl && ./config no-hw no-aria no-bf no-blake2 no-camellia no-cast no-chacha no-cmac no-des no-dh no-dsa no-ecdh no-ecdsa no-idea no-md4 no-mdc2 no-ocb no-poly1305 no-rc2 no-rc4 no-rmd160 no-scrypt no-seed no-siphash no-siv no-sm2 no-sm3 no-sm4 no-whirlpool no-asm no-ssl no-tls no-dtls no-ct no-ec no-ec2m no-fips no-gost no-ocsp no-rfc3779 no-sock no-srp no-tests no-ui -g3 && make -j
spy: spy.c openssl/libcrypto.so
	gcc -g -Os -o $@ $< -Iopenssl/include -lcrypto -Lopenssl -Wl,-rpath=openssl -Wno-incompatible-pointer-types -Wno-int-conversion
