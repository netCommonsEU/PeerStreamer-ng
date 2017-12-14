#include <malloc.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <msg_buffer.h>

static void msg_buffer_configure_ths_and_tos(struct msg_buffer *msgb,
					     uint32_t th1_to_us,
					     uint32_t th1_size,
					     uint32_t th2_to_us,
					     uint32_t th2_size,
					     uint32_t start_to_us,
					     uint32_t start_size,
					     uint32_t flush_to_us)
{
	msg_buffer_set_ths_to(msgb, th1_to_us, th2_to_us);
	msg_buffer_set_ths_size(msgb, th1_size, th2_size);
	msg_buffer_set_start_buffering_to_us(msgb, start_to_us);
	msg_buffer_set_start_buf_size_th(msgb, start_size);
	msg_buffer_set_flush_to_us(msgb, flush_to_us);
}

void msg_buffer_init_test()
{

	struct msg_buffer *msgb;

	msgb = msg_buffer_init(0, "test");
	assert(msgb != NULL);
	msg_buffer_destroy(&msgb);
	assert(msgb == NULL);

	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void msg_buffer_single_push_pop_test()
{
	struct msg_buffer *msgb;
	int i, res;
#define B1_SIZE 128
	uint8_t b1[B1_SIZE] = {0, };
	uint8_t *b1_check = NULL;
	int b1_size_check;

	for (i = 0; i < B1_SIZE; i++) {
		b1[i] = (uint8_t) rand();
	}

	msgb = msg_buffer_init(0, "test");
	assert(msgb != NULL);

	msg_buffer_configure_ths_and_tos(msgb,
					 MSG_BUFFER_TH1_TO_US,
					 0,
					 MSG_BUFFER_TH2_TO_US,
					 0,
					 0,
					 B1_SIZE >> 1,
					 60000000);

	res = msg_buffer_push(msgb, b1, (uint32_t) B1_SIZE);
	assert(res == 0);

	res = msg_buffer_get_status(msgb, NULL);
	assert(res == MSG_BUFFER_DATA_READY);

	b1_size_check = msg_buffer_pop(msgb, &b1_check);
	assert(b1_size_check == B1_SIZE);
	res = memcmp(b1, b1_check, B1_SIZE);
	assert(res == 0);

	free(b1_check);
	b1_check = NULL;

	res = msg_buffer_get_status(msgb, NULL);
	assert(res == MSG_BUFFER_DATA_NOT_READY);

	b1_size_check = msg_buffer_pop(msgb, &b1_check);
	assert(b1_size_check == 0);
	assert(b1_check == NULL);

	msg_buffer_destroy(&msgb);
	assert(msgb == NULL);

#undef B1_SIZE

	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void msg_buffer_push_pop_test()
{
	struct msg_buffer *msgb;
	int i, res;
	uint32_t current_size;

#define B1_SIZE 128
#define B2_SIZE 256
#define B3_SIZE 512

	uint8_t b1[B1_SIZE] = {0, };
	uint8_t b2[B2_SIZE] = {0, };
	uint8_t b3[B3_SIZE] = {0, };
	uint8_t *b_check = NULL;
	int b_size_check;

	for (i = 0; i < B1_SIZE; i++) {
		b1[i] = (uint8_t) rand();
	}

	for (i = 0; i < B2_SIZE; i++) {
		b2[i] = (uint8_t) rand();
	}

	for (i = 0; i < B3_SIZE; i++) {
		b3[i] = (uint8_t) rand();
	}

	msgb = msg_buffer_init(0, "test");
	assert(msgb != NULL);

	msg_buffer_configure_ths_and_tos(msgb,
					 MSG_BUFFER_TH1_TO_US,
					 0,
					 MSG_BUFFER_TH2_TO_US,
					 0,
					 0,
					 B1_SIZE >> 1,
					 60000000);

	res = msg_buffer_push(msgb, b1, (uint32_t) B1_SIZE);
	assert(res == 0);

	current_size = msg_buffer_get_current_size(msgb);
	assert(current_size = B1_SIZE);

	res = msg_buffer_push(msgb, b2, (uint32_t) B2_SIZE);
	assert(res == 0);

	current_size = msg_buffer_get_current_size(msgb);
	assert(current_size = (B1_SIZE + B2_SIZE));

	res = msg_buffer_push(msgb, b3, (uint32_t) B3_SIZE);
	assert(res == 0);

	current_size = msg_buffer_get_current_size(msgb);
	assert(current_size = (B1_SIZE + B2_SIZE + B3_SIZE));

	res = msg_buffer_get_status(msgb, NULL);
	assert(res == MSG_BUFFER_DATA_READY);

	b_size_check = msg_buffer_pop(msgb, &b_check);
	assert(b_size_check == B1_SIZE);
	res = memcmp(b1, b_check, B1_SIZE);
	assert(res == 0);
	free(b_check);
	b_check = NULL;

	current_size = msg_buffer_get_current_size(msgb);
	assert(current_size = (B2_SIZE + B3_SIZE));

	res = msg_buffer_get_status(msgb, NULL);
	assert(res == MSG_BUFFER_DATA_READY);

	b_size_check = msg_buffer_pop(msgb, &b_check);
	assert(b_size_check == B2_SIZE);
	res = memcmp(b2, b_check, B2_SIZE);
	assert(res == 0);
	free(b_check);
	b_check = NULL;

	current_size = msg_buffer_get_current_size(msgb);
	assert(current_size = (B3_SIZE));

	res = msg_buffer_get_status(msgb, NULL);
	assert(res == MSG_BUFFER_DATA_READY);

	b_size_check = msg_buffer_pop(msgb, &b_check);
	assert(b_size_check == B3_SIZE);
	res = memcmp(b3, b_check, B3_SIZE);
	assert(res == 0);
	free(b_check);
	b_check = NULL;

	current_size = msg_buffer_get_current_size(msgb);
	assert(current_size == 0);

	res = msg_buffer_get_status(msgb, NULL);
	assert(res == MSG_BUFFER_DATA_NOT_READY);

	b_size_check = msg_buffer_pop(msgb, &b_check);
	assert(b_size_check == 0);
	assert(b_check == NULL);

	msg_buffer_destroy(&msgb);
	assert(msgb == NULL);

#undef B1_SIZE
#undef B2_SIZE
#undef B3_SIZE

	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void msg_buffer_start_buffering_size_th_test()
{
	struct msg_buffer *msgb;
	int i, res;
#define START_SIZE (0x100000)
#define BN	   (8)
#define B_SIZE	   (START_SIZE / BN)
	uint8_t b[B_SIZE] = {0, };

	msgb = msg_buffer_init(0, "test");
	assert(msgb != NULL);

	msg_buffer_configure_ths_and_tos(msgb,
					 MSG_BUFFER_TH1_TO_US,
					 0,
					 MSG_BUFFER_TH2_TO_US,
					 0,
					 60000000,
					 START_SIZE,
					 60000000);

	for (i = 0; i < (BN - 1); i++) {
		res = msg_buffer_push(msgb, b, (uint32_t) B_SIZE);
		assert(res == 0);

		res = msg_buffer_get_status(msgb, NULL);
		assert(res == MSG_BUFFER_DATA_START_BUF);
	}

	res = msg_buffer_push(msgb, b, (uint32_t) B_SIZE);
	assert(res == 0);

	res = msg_buffer_get_status(msgb, NULL);
	assert(res == MSG_BUFFER_DATA_READY);

	msg_buffer_destroy(&msgb);
	assert(msgb == NULL);

#undef START_SIZE
#undef BN
#undef B1_SIZE

	fprintf(stderr,"%s successfully passed!\n",__func__);

}

void msg_buffer_start_buffering_to_test()
{
	struct msg_buffer *msgb;
	int res;
#define START_SIZE  (0x100000)
#define START_TO_US (50000)
#define BN	    (8)
#define B_SIZE	    (START_SIZE / BN)
	uint8_t b[B_SIZE] = {0, };

	msgb = msg_buffer_init(0, "test");
	assert(msgb != NULL);

	msg_buffer_configure_ths_and_tos(msgb,
					 MSG_BUFFER_TH1_TO_US,
					 0,
					 MSG_BUFFER_TH2_TO_US,
					 0,
					 START_TO_US,
					 START_SIZE,
					 60000000);

	res = msg_buffer_push(msgb, b, (uint32_t) B_SIZE);
	assert(res == 0);

	res = msg_buffer_get_status(msgb, NULL);
	assert(res == MSG_BUFFER_DATA_START_BUF);

	usleep(START_TO_US + 1000);

	res = msg_buffer_get_status(msgb, NULL);
	assert(res == MSG_BUFFER_DATA_READY);

	msg_buffer_destroy(&msgb);
	assert(msgb == NULL);

#undef START_SIZE
#undef START_TO_US
#undef BN
#undef B1_SIZE

	fprintf(stderr,"%s successfully passed!\n",__func__);

}

void msg_buffer_full_buffer_test()
{
	struct msg_buffer *msgb;
	int res;
	uint32_t nslots, nslots_check, i;
	uint8_t *b = NULL;
	uint8_t *b_check = NULL;
	uint32_t b_check_size;

	msgb = msg_buffer_init(0, "test");
	assert(msgb != NULL);

	nslots = msg_buffer_get_nslots(msgb);

	b = malloc(nslots);
	assert(b != NULL);

	for (i = 0; i < nslots; i++) {
		b[i] = (uint8_t) rand();
	}

	msg_buffer_configure_ths_and_tos(msgb,
					 MSG_BUFFER_TH1_TO_US,
					 0,
					 MSG_BUFFER_TH2_TO_US,
					 0,
					 0,
					 0,
					 60000000);

	for (i = 0; i < nslots; i++) {
		res = msg_buffer_push(msgb, &b[i], 1);
		assert(res == 0);
	}

	nslots_check = msg_buffer_get_nslots(msgb);
	assert(nslots == nslots_check);

	res = msg_buffer_get_status(msgb, NULL);
	assert(res == MSG_BUFFER_DATA_READY);

	for (i = 0; i < nslots; i++) {
		b_check_size = msg_buffer_pop(msgb, &b_check);
		assert(b_check_size == 1);
		res = memcmp(&b[i], b_check, 1);
		assert(res == 0);

		free(b_check);
		b_check = NULL;
	}

	msg_buffer_destroy(&msgb);
	assert(msgb == NULL);

	free(b);

	fprintf(stderr,"%s successfully passed!\n",__func__);

}

void msg_buffer_increase_nslots_test()
{
	struct msg_buffer *msgb;
	int res;
	uint32_t nslots, new_nslots, i;
	uint8_t *b = NULL;
	uint8_t *b_check = NULL;
	uint32_t b_check_size;

	msgb = msg_buffer_init(0, "test");
	assert(msgb != NULL);

	nslots = msg_buffer_get_nslots(msgb);

	b = malloc(nslots * 2);
	assert(b != NULL);

	for (i = 0; i < (nslots * 2); i++) {
		b[i] = i;
	}

	msg_buffer_configure_ths_and_tos(msgb,
					 MSG_BUFFER_TH1_TO_US,
					 0,
					 MSG_BUFFER_TH2_TO_US,
					 0,
					 0,
					 0,
					 60000000);

	for (i = 0; i < (nslots * 2); i++) {
		res = msg_buffer_push(msgb, &b[i], 1);
		assert(res == 0);
	}

	new_nslots = msg_buffer_get_nslots(msgb);
	assert((nslots * 2) == new_nslots);

	res = msg_buffer_get_status(msgb, NULL);
	assert(res == MSG_BUFFER_DATA_READY);

	for (i = 0; i < new_nslots; i++) {
		b_check_size = msg_buffer_pop(msgb, &b_check);
		assert(b_check_size == 1);
		res = memcmp(&b[i], b_check, 1);
		assert(res == 0);

		free(b_check);
		b_check = NULL;
	}

	msg_buffer_destroy(&msgb);
	assert(msgb == NULL);

	free(b);

	fprintf(stderr,"%s successfully passed!\n",__func__);

}

void msg_buffer_flush_test()
{
	struct msg_buffer *msgb;
	int res, i;
#define START_SIZE  (0x100000)
#define START_TO_US (50000)
#define FLUSH_TO_US (50000)
#define BN	    (8)
#define B_SIZE	    (START_SIZE / BN)
	uint8_t b[B_SIZE] = {0, };
	uint8_t *b_check = NULL;

	msgb = msg_buffer_init(0, "test");
	assert(msgb != NULL);

	msg_buffer_configure_ths_and_tos(msgb,
					 MSG_BUFFER_TH1_TO_US,
					 0,
					 MSG_BUFFER_TH2_TO_US,
					 0,
					 START_TO_US,
					 START_SIZE,
					 FLUSH_TO_US);

	for (i = 0; i < BN; i++) {
		res = msg_buffer_push(msgb, b, (uint32_t) B_SIZE);
		assert(res == 0);
	}

	res = msg_buffer_get_status(msgb, NULL);
	assert(res == MSG_BUFFER_DATA_READY);

	res = msg_buffer_is_flushing(msgb);
	assert(res == 0);

	usleep(FLUSH_TO_US + 1000);

	res = msg_buffer_get_status(msgb, NULL);
	res = msg_buffer_is_flushing(msgb);
	assert(res == 1);

	for (i = 0; i < BN; i++) {
		msg_buffer_pop(msgb, &b_check);

		free(b_check);
		b_check = NULL;
	}

	res = msg_buffer_get_status(msgb, NULL);
	assert(res == MSG_BUFFER_DATA_FLUSHED);

	msg_buffer_destroy(&msgb);
	assert(msgb == NULL);

#undef START_SIZE
#undef START_TO_US
#undef FLUSH_TO_US
#undef BN
#undef B1_SIZE

	fprintf(stderr,"%s successfully passed!\n",__func__);

}

int main(int argc, char ** argv)
{
	msg_buffer_init_test();
	msg_buffer_single_push_pop_test();
	msg_buffer_push_pop_test();
	msg_buffer_start_buffering_size_th_test();
	msg_buffer_start_buffering_to_test();
	msg_buffer_full_buffer_test();
	msg_buffer_increase_nslots_test();
	msg_buffer_flush_test();
	/* TODO:
	 * - Better tests for mixed push/pop
	 * - th1, th2 thresholds/timeouts tests
	 * - msg_buffer_start_buf_reinit test
	 * - parse mechanism test
	 */
	return 0;
}
