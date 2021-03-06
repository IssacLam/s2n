/*
 * Copyright 2016 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include "s2n_test.h"

#include <inttypes.h>
#include <fcntl.h>
#include <s2n.h>

#include <openssl/aes.h>

#include "crypto/s2n_drbg.h"

#include "utils/s2n_safety.h"
#include "utils/s2n_random.h"
#include "utils/s2n_timer.h"

#include "tls/s2n_config.h"

#include "testlib/s2n_testlib.h"


/* Test vectors are taken from http://csrc.nist.gov/groups/STM/cavp/documents/drbg/drbgtestvectors.zip
 * - drbgvectors_pr_true/CTR_DRBG.txt :
 * [AES-128 no df]
 * [PredictionResistance = True]
 * [EntropyInputLen = 256]
 * [NonceLen = 0]
 * [PersonalizationStringLen = 256]
 * [AdditionalInputLen = 0]
 * [ReturnedBitsLen = 512]
 */

struct s2n_stuffer nist_reference_entropy;
const char nist_reference_entropy_hex[] =
"528ccd2d6c143800a34ad33e7f153cfaceaa2411abbaf4bfcfe9796898d0ece6"
"478fd1eaa7ed293294d370979b0f0f1948d5a3161b12eeebf2cf6bd1bf059adf"
"036bf1173977b323f60c6f0f603d9c50835b2afca8347dfb24e8f66604444951"
"53e5783fd27cda264189ff0aa5513dc5903c28e3e260b77b4b4a0b9b76e4aa46"
"e04bade7636e72397f8303437cb22be52810dadd768d5cfc9d3269e7ad4bc8bc"
"e20dabb9fcaf3aed0ec40c7ef68d78f2bcb5675db2d62ee40d8184b7046f0e0e"
"f3b247d0400740d18b795e8e9c04cc43eb138cb9eb6c46862030517d8b3679db"
"0603b481bb88d77d6bacf03a64cf82248420a95cca10e3efc012bd770dd5621c"
"233b1e550a0d1249a5bd09ccb9a88be9119258feadfc309a6ae3c340918545b9"
"9d97ce8452474bc19a4be64efd43484874074779e05ac221f19db3955ce29bc1"
"5c0ef27b53eb7903cc9e2bc709f9e885eec139ec687a3444450f0cb079ca5171"
"0e26d3e8d2fd18fe0ce2538c2ede5d61574c8227b3f82b333ac42dbdd78b8e19"
"12f68f049216e3987860651f6fd13efef184fc8fe05cac459eb3cf97dd957e6c"
"ccfabc0f847801b342774ce50d80c600af2d1f2016e1aea412fc4c60b2a041b1"
"07e192e576bb031bb8baa60b9374dcc1c40cb43ab42035719588e2afd75fc8f0"
"6129a163efde5f14067e075c4790029652a56eedf8ea4877a0ffcedd32866166"
"356ade73b8ffb38408be0537ca539b633d0afffcf2962b717ac37eb2a06ddf50"
"0b857fe137cab149e9195019732a714e25e62c946eba58dd2fa8e6c797172bcb"
"310d2e7b249ca8fdf1d4b15612f6e1f5428241c7ad777281171fdcecf7f5ed44"
"241e8fe2d6de5efb6509144c8dba00a582d1aebfabd306052747fc311d7c5a56"
"a2c100da70d8659d94a4d6a0f10594c8412c977168df242a35d5c2600a47084b"
"f5771eeacddef0c6cbfac5ac4285205f110fd6a8174ad45748a8633a28c85ec2"
"0621babc5c0ea527ca7c8d42338e16b6113b387ba888a6c12779cc127561a148"
"672df1d58510c016913fc6c42771273f7cbaeef1993dbabcd6cca26758bdb7df"
"7a9365ebb48fdd83809167cb2bfebc37b69b34595b2f8d81cff2924a70e300d5"
"954e64b6f4f1d7e0135eb2a9174612d23111329c00bded4cecf6b2b2fdfad145"
"e6495728cb28b8f64b208afa9e1012f061c790d285663f5cf0760c9f7c4bd32f"
"bd10e4b282c98b6c2e0451272a332c24f54a2a45244e9889420a48786f662aed"
"5a1cf5b5212ae2a776ae4f688ea9e6124f624c817b5e429b8cc2ad730cf29f6e"
"fcca996bf011ba139b7bd05e31c594d459685aa143e0ee46d6db0ba8582a6fd6"
"9009b2d3b9ef7fd1e925d75d6ad07d42e165310153ed1ef9e1284894e0d16575"
"13f8641a1d9cd50966b4fff6f2a5a9f67aefa8d7b5cb527d3b73338aea773ee1"
"4645aa5ebb41f27380328161864cca36055c871356316bf3773d35687fe72874"
"87e58c3c91ae0935b7935d2d0a855e795327062238180f20b02f9c9c861cbc33"
"28c5691d0c0ff0d834cb0c85a82fe561f40803f63beb3e740215e5d5089fc554"
"fd22324369fd30e2f47553c012ce2c46fd40df5fcdd737bf266cd20bcdc20a98"
"f3291f1d5fa47abf1918416a2cb78b243e614f883bda2afc10ffc1cc7f7f06a8"
"c9cbe9f41e2d81f99ca6327ca617caf836b73512d96b1edc5ae5ed56c056cade"
"47ebfc4437f769ac299f0d87069e5d6074420113761a9f9e053866581e836701"
"bcb51bc0dfae0fb04bf08face7369981d9a78bbcc0d3da2a1442d8fef756c1c0"
"b74a392be091fe00eb6a6576f027073a838308f5a5aed7fd0db47d6616cbec98"
"d39a307b956d1183f718be7d91a11891dc3fee45652edb89449d9766fc85bb32"
"5565457cf60a3aff9ea6f3dc6254a76a085ff87f1dd19127b2aeb3ac50b8c31e"
"9dfd31e3adc822b675c0a9c8702df12de4c3354ccdb5389ed481d910b3dbb51a"
"2a1b1519d22d40ef4ec23c6d97dc148cfe171fb5f8b1c305ec6d8e83a1ef9064";

int entropy_fd = -1;

const char nist_reference_personalization_strings_hex[] =
"07ca7a007b53f014d7f10461d6c97b5c8b0c502d3461d583f99efec69f121cd2"
"99f2d501dd183f5657d8b8061bf4f0988de3bc2719f41281cf08d9bcc99f0dcb"
"617d6463eaf3849e13b89a7f9805edff3ea1f0aa7ad71e7de7af3481bfeba7f5"
"7f7d81a5c72f9e3498de183329d6b2291792ac8c81e690200387305aba8987c7"
"0c70a20b690bf5d586b284aa752ec73055e039233089a30efdd6218a1e49f548"
"1fe2b0ce3a1896017e30753064e1c0e1341b673569c739a199cd218fe64665d0"
"2703e1dca9926995705229cc8a8c9d958bcdbfd5f3e09eeb349b5135686aeed1"
"9ca4b8e6cdec1ecd9546508542c84936ff2aa1a4dab969613139c01e35f36d71"
"0f533937857dc9f97fefa142c95445b484e1f259a488e9fa38a46e49d693c4b6"
"950472ba4b3b6979ba4d2a1ad51db7732eb3337b28270fdb7f99018144734f72"
"06bd36977e09cc13d7c9edc9a285043f7c00575610e9b95fd3754a4b7c561fb7"
"6dacec09cc407ddf545c4d54c77efd712f65be8ba5ecbc9fa6bdf29243eed72e"
"85facc571d2c6eb257437e1d3b77b5fba4a125606440181b8387f9a4fb1a7fbf"
"2c4e73ad2597b027dd04c6056db1189b7a0d8e9fece52237cb7fc2511509c4eb"
"717c4de30c0ac7b831807cb1fcf4ab37f1734683a723d183fcfe5fc8a4c6c7e5";

const char nist_reference_values_hex[] =
"462eaef2ff6d82aec55f451776700e4c" "010cd7c293306adbe9798f2f65bdfb01" "f6f808dd7e199b3ce8497d63515092df"
"1e574e0a9b220668776a109ecec959f5" "f47380b9e6d8ce485a5bb9f890331f89" "f2b475b29ed8aca7f3a69477212153e3"
"d63aa6ddf10dfb6934b7a745456f2056" "29c05402d0dd6ff1d171c523e6066b3d" "fb15e2dfd607439797e29f9ea9a24788"
"601d313b010af1930132417697d9e27e" "b17422b4a74cc83de34c7196cd232355" "b63cf4e0894e185eb7ef572c2adacf98"
"a7ec1f62b063acd9904d2ca4b26e755c" "b98e1a708027be7d5f0ff46f775ceece" "5b5adf71f7d8c85a011acc778c4e3d6f"
"6536d316f19bd244ca1a2deba572face" "0ce6ba1487b38261f306c84642338b53" "a19af0020f46085ca3caf34b2cb4f1f5"
"cac724dc3e214ff8d0ac4f60ee2dfded" "67138c2c7a488b2f4449b03192aa54f8" "d10073624d73847284c91b46e28b83c3"
"edadadc2ad451ea48ab9619d6c89cdcb" "24f64c09499b4803a03aadf0e34240ca" "8ac5b02284cac91bb39e14bd3d38ae2b"
"31f21cce9f11c7e9047e3ebad7c23a1b" "6b59014f0e7e8df5f7456017841e9c63" "4cfc026d6bc19e9674ec7002bf0baf62"
"d871c3f06cdf34c0cebb8b405aa79be7" "6af76a8c51f305b88249af9db4eacfe4" "ff77327ab3761736e40f79ca0a6660e2"
"9eedbc9923b20434c175c066ed3584ba" "06ebe4bd2ea20c39ef8f4f66796816bb" "497b437274f3d7cee04b163c10233aec"
"7fca6267fd42102de5baacb7b4409565" "1310719710bd2c3468f8000fad5b6f4c" "34e5d8e0f7e6c28718a5f27ac086516b"
"9948b0263f2c91756050fad1f5d7876f" "9aff44fbc0f539186d13e81a9b233786" "0a86ad5766efe6d3fa01a9ac4ec9090d"
"a022dfed4c805b8f2c15d81693edfb53" "9d4f527ab03ff0c4abe7e442572fc0bb" "5fc9cb9e3cd694c6b08bb99256062814"
"faa46432da44e336bd782edd85ccfa83" "c17b86c7fd8e5a2131f515fb0ab62c9b" "619ff6d15579a1cb4bc9c96f6be8f4e5";

const char nist_reference_returned_bits_hex[] =
"bba1ef50b4bad288897f02ac2706ee1e01488dcbe9b3d8a637921f5e788faed3b23db63d590ceaa7607a2179192bea9aeaa85d048e7e108fee666dc646af5f0e"
"b9ecf5521d38f9212578f9dd32af38089b45812ca96e661fc8e1be0b2f4b54215f52c9c93a6f76efee4521cb316378403108a6bf2724fdc93bc4d2db60a2de83"
"ce4bec8241dd1ce12d7fa18bd1a3188b43892392b7dcae7228a851afef9c9728c587167b6df28c895a67af35b6fd84ad076efcd70d1c59c49fa7c0f7ed87bc82"
"94001b011830bc6a911fa23bc1fd0dac8c0acd0e856ab4497ad072e54fe3b47bc06890e7e7babd4912af1e7da96e90523b75dd86d9a4ba4d88107de607e52534"
"d0619996c88423a740f04dfdad3bfc131bf3601fc43022e89957bf8b9a1ad8702bc8fbb2004e7ab02550777818e11f03fab48458424ae5c8cc8cb73436657292"
"a2613ddb75107ef3fe9fc392061a52ea2304af7aed6dedec3aee626e1ab2bf989ce7d6484555941e9a4013036c4605c564f17d4d1369e1e12d28b877d2125d99"
"48460543d08b3331f0c7bcf786be06a276efc757bedce0f428323d54206931f18c3c7c606c0ff8e673f3d1367c7d50db2ed002120ff05623d2deddd037c125a4"
"037a0305532b6bd2a51057962f23dbfd4660be8e5b7a448b1168be3bf8598a301fb517a4714b7826162fc0fcdae08800e967f638ff1ad1da39282d3454d93075"
"035c19396d603b52222c16af6c6bc1c07a1a518f578b59943ffad73ca14948b7a8dde117e5c571506d57fd08e3a067a2ae3bde2240c2399f160c5cc5a2f5d582"
"e17674172397369025d6b7811b69b6e62dfb8ab94852cc96bde1371fcedbe5fe31a11589f4e57183fb46883d93c647e36f70f8d5a536f8fb0d428dcfd7722e4e"
"cbedccaa88b22e39fb0e14bf15dc05e4e1b7002fa0cffe7803e3f6bcf6a03f3faa51ce5b3ccb0d341533d335e20f9f71e90f83fef06ecfe93ed056f5d6851306"
"116bd7ab4030379cd6f50e85a04182775292fa9619c38b7418a19b8e7855f29efffdf7a2b8b1d9d3d96ea85a6d56302014dd3bcbe401ada5b0e3cf2f66dce9cd"
"51e0b54c109884baecd1884f7bafc846ace216b6fd97eb1ca70b563e62c4a2f22b55561152f379326ef2999e9746f25043a02402d3e47b4a58e747c222b7a081"
"7c41adf941656cfb9f24409d6cc4d578d43930b3e23ec801a59c53d999401bc0cb3e5b8797b2770a8a8f51ff594b7b17d9e694d5e36644508d16cb2554057adc"
"ac054570b081cf53b39b0a2faa21ee9b554c05ff9055843ac0eb9031d1de324701ad4cf2875623e0bf4184de4aea20070be1cb586880ac87fbb7e414b4b128d0";


/* This function over-rides the s2n internal copy of the same function */
int nist_fake_urandom_data(struct s2n_blob *blob)
{
    /* At first, we use entropy data provided by the NIST test vectors */
    GUARD(s2n_stuffer_read(&nist_reference_entropy, blob));

    return 0;
}

int main(int argc, char **argv)
{
    uint8_t data[256] = { 0 };
    struct s2n_drbg drbg = {{ 0 }};
    struct s2n_blob blob = {.data = data, .size = 64 };
    struct s2n_timer timer;
    uint64_t drbg_nanoseconds;
    uint64_t urandom_nanoseconds;
    struct s2n_stuffer nist_reference_personalization_strings;
    struct s2n_stuffer nist_reference_returned_bits;
    struct s2n_stuffer nist_reference_values;
    struct s2n_config *config;

    BEGIN_TEST();

    EXPECT_NOT_NULL(config = s2n_config_new())

    /* Open /dev/urandom */
    EXPECT_TRUE(entropy_fd = open("/dev/urandom", O_RDONLY));

    /* Convert the hex entropy data into binary */
    EXPECT_SUCCESS(s2n_stuffer_alloc_ro_from_hex_string(&nist_reference_entropy, nist_reference_entropy_hex));
    EXPECT_SUCCESS(s2n_stuffer_alloc_ro_from_hex_string(&nist_reference_personalization_strings, nist_reference_personalization_strings_hex));
    EXPECT_SUCCESS(s2n_stuffer_alloc_ro_from_hex_string(&nist_reference_returned_bits, nist_reference_returned_bits_hex));
    EXPECT_SUCCESS(s2n_stuffer_alloc_ro_from_hex_string(&nist_reference_values, nist_reference_values_hex));

    /* Check everything against the NIST vectors */
    for (int i = 0; i < 14; i++) {
        uint8_t ps[32];
        struct s2n_drbg nist_drbg = { .entropy_generator = nist_fake_urandom_data };
        struct s2n_blob personalization_string = {.data = ps, .size = 32};
        /* Read the next personalization string */
        EXPECT_SUCCESS(s2n_stuffer_read(&nist_reference_personalization_strings, &personalization_string));

        /* Instantiate the DRBG */
        EXPECT_SUCCESS(s2n_drbg_instantiate(&nist_drbg, &personalization_string));

        uint8_t nist_v[16];

        GUARD(s2n_stuffer_read_bytes(&nist_reference_values, nist_v, sizeof(nist_v)));
        EXPECT_TRUE(memcmp(nist_v, nist_drbg.v, sizeof(nist_drbg.v)) == 0);

        /* Generate 512 bits (FIRST CALL) */
        uint8_t out[64];
        struct s2n_blob generated = {.data = out, .size = 64 };
        EXPECT_SUCCESS(s2n_drbg_generate(&nist_drbg, &generated));

        GUARD(s2n_stuffer_read_bytes(&nist_reference_values, nist_v, sizeof(nist_v)));
        EXPECT_TRUE(memcmp(nist_v, nist_drbg.v, sizeof(nist_drbg.v)) == 0);

        /* Generate another 512 bits (SECOND CALL) */
        EXPECT_SUCCESS(s2n_drbg_generate(&nist_drbg, &generated));

        GUARD(s2n_stuffer_read_bytes(&nist_reference_values, nist_v, sizeof(nist_v)));
        EXPECT_TRUE(memcmp(nist_v, nist_drbg.v, sizeof(nist_drbg.v)) == 0);

        uint8_t nist_returned_bits[64];
        GUARD(s2n_stuffer_read_bytes(&nist_reference_returned_bits, nist_returned_bits, sizeof(nist_returned_bits)));
        EXPECT_TRUE(memcmp(nist_returned_bits, out, sizeof(nist_returned_bits)) == 0);

        EXPECT_SUCCESS(s2n_drbg_wipe(&nist_drbg));
    }

    EXPECT_SUCCESS(s2n_drbg_instantiate(&drbg, &blob));

    /* Use the DRBG for 32MB of data */
    EXPECT_SUCCESS(s2n_timer_start(config, &timer));
    for (int i = 0; i < 500000; i++) {
        EXPECT_SUCCESS(s2n_drbg_generate(&drbg, &blob));
    }
    EXPECT_SUCCESS(s2n_timer_reset(config, &timer, &drbg_nanoseconds));

    /* Use urandom for 32MB of data */
    EXPECT_SUCCESS(s2n_timer_start(config, &timer));
    for (int i = 0; i < 500000; i++) {
        EXPECT_SUCCESS(s2n_get_urandom_data(&blob));
    }
    EXPECT_SUCCESS(s2n_timer_reset(config, &timer, &urandom_nanoseconds));

    /* Confirm that the DRBG is faster than urandom when rdrand is enabled */
    if (s2n_cpu_supports_rdrand()) {
        EXPECT_TRUE(drbg_nanoseconds < urandom_nanoseconds);
    }

    /* NOTE: s2n_random_test also includes monobit tests for this DRBG */

    /* the DRBG state is 128 bytes, test that we can get more than that */
    blob.size = 129;
    for (int i = 0; i < 10; i++) {
        EXPECT_SUCCESS(s2n_drbg_generate(&drbg, &blob));
    }

    EXPECT_SUCCESS(s2n_drbg_wipe(&drbg));

    EXPECT_SUCCESS(s2n_stuffer_free(&nist_reference_entropy));
    EXPECT_SUCCESS(s2n_stuffer_free(&nist_reference_personalization_strings));
    EXPECT_SUCCESS(s2n_stuffer_free(&nist_reference_returned_bits));
    EXPECT_SUCCESS(s2n_stuffer_free(&nist_reference_values));

    END_TEST();
}
