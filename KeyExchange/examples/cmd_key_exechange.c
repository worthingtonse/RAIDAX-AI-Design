/* ====================================================
#   Copyright (C) 2023 CloudCoinConsortium 
#
#   Author        : Alexander Miroch
#   Email         : alexander.miroch@protonmail.com
#   File Name     : cmd_key_exechange.c
#   Last Modified : 2024-12-15 11:03
#   Describe      : Healing Commands
#
# ====================================================*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>


#include "protocol.h"
#include "log.h"
#include "commands.h"
#include "db.h"
#include "config.h"
#include "utils.h"
#include "net.h"
#include "aes.h"


extern struct ticket_entry_t tickets[TICKET_POOL_SIZE];
extern struct config_s config;

/*
 * Encrypt Key Command
 */
void cmd_encrypt_key(conn_info_t *ci) {
  unsigned char *payload = get_body_payload(ci);
  int coin_length, total_coins;
  uint32_t sn;
  int i, j;
  int8_t den;
  int sn_idx;
  struct page_s *page;
  int r0, r1;
  struct ticket_entry_t *te;
  unsigned char aen[16];

  debug("CMD Encrypt Key");

  // 16CH + (DN + SN + 8KY) = 13 + 2EOF
  if (ci->body_size != 31) {
    error("Invalid command length: %d. Need 31", ci->body_size);
    ci->command_status = ERROR_INVALID_PACKET_LENGTH;
    return;
  }

  // Get DN and SN of who the key is for
  den = ((uint8_t) payload[0]);
  sn = get_sn(&payload[1]);

  // Get page
  page = get_page_by_sn_lock(den, sn);
  if (page == NULL) {
    error("Invalid sn or denomination passed for coin %d, sn %d -> %hhx", i, sn, den);
    ci->command_status = ERROR_INVALID_SN_OR_DENOMINATION;
    return;
  }

  sn_idx = sn % RECORDS_PER_PAGE;
  memcpy(aen, &page->data[sn_idx * 17], 16);
  unlock_page(page);

  debug("Loaded coin %hhx:%u AN %02x%02x%02x ... %02x%02x", den, sn, aen[0], aen[1], aen[2], aen[14], aen[15]);

  ci->output_size = 16;
  ci->output = (unsigned char *) malloc(ci->output_size);
  if (ci->output == NULL) {
    error("Can't alloc buffer for the response");
    ci->command_status = ERROR_MEMORY_ALLOC;
    return;
  }

  memset(ci->output, 0, 16);

  // Copy first 8 bytes
  memcpy(ci->output, &payload[5], 8);

  // Get two 4-byte integers
  srand(time(NULL));
  r0 = rand();

  //srand(time(NULL));
  //r1 = rand();

  // 5 byte DN + SN (payload[1] is the SN)
  ci->output[8] = den;
  memcpy(ci->output + 8 + 1, &payload[1], 4);
  
  ci->output[8 + 1 + 4] = (r0 >> 8) & 0xff;
  ci->output[8 + 1 + 4 + 1] = r0 & 0xff;

  // last byte
  ci->output[15] = 0xff;

  debug("Generated %02x%02x%02x%02x%02x%02x%02x%02x %02x%02x%02x%02x%02x%02x%02x%02x", ci->output[0], ci->output[1], ci->output[2],
    ci->output[3], ci->output[4], ci->output[5], ci->output[6], ci->output[7], ci->output[8], ci->output[9],
    ci->output[10], ci->output[11], ci->output[12], ci->output[13], ci->output[14], ci->output[15]);


  crypt_ctr(aen, ci->output, 16, ci->nonce);

  ci->command_status = (char) NO_ERROR;

  debug("CMD Encrypt Key finished");
}

/*
 * Decrypts messages from RAIDA servers
 */
void cmd_decrypt_raida_key(conn_info_t *ci) {
  unsigned char *payload = get_body_payload(ci);
  int coin_length, total_coins;
  uint32_t sn;
  int i;
  int8_t den;
  int sn_idx;
  struct page_s *page;
  int an, pan, p, f, rv;
  uint8_t da;
  unsigned char *ky;
  unsigned char aens[16 * 25];
  unsigned char *aen;

  uint8_t dec_den;
  uint32_t dec_sn;

  uint8_t split_id;

  uint8_t mfs;


  debug("CMD Encrypt POST Key");

  // 16CH + DN + 4SN + (at least one 2CO + SP + RA + SH + DN + 4SN + 16KY) = 26 + 2EOF
  if (ci->body_size < 49) {
    error("Invalid command length: %d. Need at least 49", ci->body_size);
    ci->command_status = ERROR_INVALID_PACKET_LENGTH;
    return;
  }

  coin_length = ci->body_size - 23;
  if (coin_length % 26) {
    error("Can't determine the number of coins");
    ci->command_status = ERROR_COINS_NOT_DIV;
    return;
  }

  total_coins = coin_length / 26;
  debug("Requested %d coins to auth", total_coins);

  // Get DN and SN of who the key is for
  den = ((uint8_t) payload[0]);
  sn = get_sn(&payload[1]);

  mfs = get_mfs();

  debug("Coin used for decryption %hhx:%u", den, sn);
  rv = load_my_enc_coin(den, sn, aens);
  if (rv < 0) {
    error("Invalid sn or denomination passed for coin %d, sn %d -> %hhx", i, sn, den);
    ci->command_status = ERROR_COIN_LOAD;
    return;
  }

  // possible buf for mixed response. It is freed by another function
  // don't set output size here
  ci->output = (unsigned char *) malloc(total_coins);
  if (ci->output == NULL) {
    error("Can't alloc buffer for the response");
    ci->command_status = ERROR_MEMORY_ALLOC;
    return;
  }

  // all zeroes (failed coins)
  memset(ci->output, 0, total_coins);



  for (i = 0; i < total_coins; i++) {
    split_id = ((uint8_t) payload[21 + i * 26 + 2]);
    da = ((uint8_t) payload[21 + i * 26 + 3]);
    den = ((uint8_t) payload[21 + i * 26 + 5]);
    sn = get_sn(&payload[21 + i * 26 + 6]);
    ky = (unsigned char *) &payload[21 + i * 26 + 10];

    if (da > 24) {
      error("Invalid Raida passed for coin %d, sn %d -> %hhx. Raida %d Skipping it", i, sn, den, da);
      ci->output[i] = 0x0;
      f++;
      continue;
    }

    if (split_id != 0 && split_id != 1) {
      error("Invalid split_id passed for coin %d, sn %d -> %hhx. Split %d Skipping it", i, sn, den, split_id);
      ci->output[i] = 0x0;
      f++;
      continue;
    }


    debug("den %hhx, SN %u, Da %u, Split %u", den, sn, da, split_id);
    page = get_page_by_sn_lock(den, sn);
    if (page == NULL) {
      error("Invalid sn or denomination passed for coin %d, sn %d -> %hhx. Skipping it", i, sn, den);
      ci->output[i] = 0x0;
      f++;
      continue;
    }

    aen = &aens[da * 25];
    debug("KY %02x%02x%02x%02x%02x%02x%02x%02x ... %02x%02x ", ky[0], ky[1], ky[2], ky[3], ky[4], ky[5], ky[6], ky[7], ky[14], ky[15]);
    debug("MY AN %02x%02x%02x%02x%02x%02x%02x%02x ... %02x%02x ", aen[0], aen[1], aen[2], aen[3], aen[4], aen[5], aen[6], aen[7], aen[14], aen[15]);

    crypt_ctr(&aens[da], ky, 16, ci->nonce);
    debug("KY DECRYPTED %02x%02x%02x%02x%02x%02x%02x%02x ... %02x%02x ", ky[0], ky[1], ky[2], ky[3], ky[4], ky[5], ky[6], ky[7], ky[14], ky[15]);

    if (ky[15] != 0xff) {
      error("Malformed coin AN. Can't decrypt it properly");
      ci->output[i] = 0x0;
      f++;
      continue;
    }

    dec_den = (uint8_t) ky[8];
    dec_sn = get_sn(&ky[9]);

    if (dec_den != den || dec_sn != sn) {
      error("Decrypted coin %hhx:%u does not match what was sent %hhx:%u", dec_den, dec_sn, den, sn);
      ci->output[i] = 0x0;
      f++;
      continue;
    }

    page = get_page_by_sn_lock(den, sn);
    if (page == NULL) {
      error("Failed to load page for %hhx:%u", den, sn);
      ci->output[i] = 0x0;
      f++;
      continue;
    }

    sn_idx = sn % RECORDS_PER_PAGE;
    memcpy(&page->data[sn_idx * 17 + split_id * 8], ky, 8);
    page->data[sn_idx * 17 + 16] = mfs;
    page->is_dirty = 1;

    unlock_page(page);

    ci->output[i] = 0x1; // accepted
    p++;

  }

  ci->command_status = (char) STATUS_SUCCESS;
  ci->output_size = total_coins;

  debug("Accepted %d, failed %d", p, f);

  debug("CMD POST Key Finished");
}


/*
 * 
 * Loads my encryption coin
 *
 */

int load_my_enc_coin(uint8_t den, uint32_t sn, unsigned char *buf) {
  char path[PATH_MAX];
  char tmp_buf[440];
  uint16_t coin_id;
  int fd, rv;

  debug("Loading encryption coin %hhx:%u", den, sn);

  sprintf((char *) &path, "%s/coins/%02hhx.%u.bin", config.cwd, den, sn);
  fd = open(path, O_RDONLY);
  if (fd < 0) {
    error("Failed to open page file %s: %s", path, strerror(errno));
    return -1;
  }

  rv = read(fd, tmp_buf, 440);
  if (rv < 0) {
    error("Failed to read coin file %s: %s", path, strerror(errno));
    close(fd);
    return -1;
  }

  if (rv != 440) {
    error("Invalid coin size: %d", rv);
    close(fd);
    return -1;
  }

  close(fd);

  coin_id = tmp_buf[2] << 8 | tmp_buf[3];
  if (coin_id != config.coin_id) {
    error("Invalid coin id in the file: %u. We work with %u", coin_id, config.coin_id);
    return -1;
  }

  memcpy(buf, tmp_buf + 40, 400);
  
  debug("Loaded encryption coin %hhx:%u", den, sn);

  return 0;
}












// Chat functions
void cmd_post_key(conn_info_t *ci) {
  unsigned char *payload = get_body_payload(ci);
  char key_path[PATH_MAX];

  int fd, rv;
  int8_t den;
  uint32_t sn;
  uint8_t kl, ks;

  debug("CMD POST Key");

  // KY is always 128
  // 16CH + 16KeyID + 16IP + DN + 4SN + 128KY +KS + KL + 2EOF = 185
  if (ci->body_size != 185) {
    error("Invalid command length: %d. Need 185", ci->body_size);
    ci->command_status = ERROR_INVALID_PACKET_LENGTH;
    return;
  }

  // Get DN and SN of the sender
  den = ((uint8_t) payload[32]);
  sn = get_sn(&payload[33]);

  debug("Encryption coin %hhx:%u", den, sn);

  ks = payload[165];
  kl = payload[166];

  if (ks + kl > 127) {
    error("Invalid key start %d", ks);
    ci->command_status = ERROR_INVALID_KEY_START;
    return;
  }

  sprintf((char *) &key_path, "%s/Keys/%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", config.cwd, payload[0], payload[1], payload[2], payload[3],
      payload[4], payload[5], payload[6], payload[7], payload[8], payload[9], payload[10], payload[11], payload[12], payload[13], payload[14], payload[15]);

  debug("Saving key to %s", key_path);
  
  fd = open(key_path, O_CREAT | O_WRONLY, 0640);
  if (fd < 0) {
    error("Failed to open file %s: %s", key_path, strerror(errno));
    ci->command_status = ERROR_FILESYSTEM;
    return;
  }

  // DN
  rv = write(fd, (unsigned char *) &den, 1);
  if (rv != 1) {
    error("Failed to write to file %s: %s", key_path, strerror(errno));
    ci->command_status = ERROR_FILESYSTEM;
    close(fd);
    return;
  }

  // SN
  rv = write(fd, (unsigned char *) &payload[33], 4);
  if (rv != 4) {
    error("Failed to write to file %s: %s", key_path, strerror(errno));
    ci->command_status = ERROR_FILESYSTEM;
    close(fd);
    return;
  }

  rv = write(fd, &payload[37 + ks], kl);
  if (rv != kl) {
    error("Failed to write to file %s: %s", key_path, strerror(errno));
    ci->command_status = ERROR_FILESYSTEM;
    close(fd);
    return;
  }

  ci->command_status = (char) STATUS_SUCCESS;
  close(fd);

  debug("CMD POST Key Finished");
}

void cmd_get_key(conn_info_t *ci) {
  unsigned char *payload = get_body_payload(ci);
  char key_path[PATH_MAX];

  char buf[512];

  int fd, rv;
  int8_t den;
  uint32_t sn;

  debug("CMD GET Key");
  // KY is always 128
  // 16CH + 16KeyID + DN + 4SN + 16IP + 2EOF = 55
  if (ci->body_size != 55) {
    error("Invalid command length: %d. Need 55", ci->body_size);
    ci->command_status = ERROR_INVALID_PACKET_LENGTH;
    return;
  }

  // Get DN and SN of who the key is for
  den = ((uint8_t) payload[16]);
  sn = get_sn(&payload[17]);
  debug("Encryption coin %hhx:%u", den, sn);

  sprintf((char *) &key_path, "%s/Keys/%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", config.cwd, payload[0], payload[1], payload[2], payload[3],
      payload[4], payload[5], payload[6], payload[7], payload[8], payload[9], payload[10], payload[11], payload[12], payload[13], payload[14], payload[15]);

  debug("Loading key from %s", key_path);

  fd = open(key_path, O_CREAT | O_RDONLY, 0640);
  if (fd < 0) {
    error("Failed to open file %s: %s", key_path, strerror(errno));
    ci->command_status = ERROR_FILESYSTEM;
    return;
  }

  rv = read(fd, &buf, 512);
  if (rv < 0) {
    error("Failed to read from file %s: %s", key_path, strerror(errno));
    ci->command_status = ERROR_FILESYSTEM;
    close(fd);
    return;
  }

  debug("Read %d bytes", rv);

  close(fd);

  ci->output = (unsigned char *) malloc(rv);
  if (ci->output == NULL) {
    error("Can't alloc buffer for the response");
    ci->command_status = ERROR_MEMORY_ALLOC;
    return;
  }

  ci->command_status = (char) STATUS_SUCCESS;
  ci->output_size = rv;
  memcpy(ci->output, buf, rv);

  debug("CMD GET Key Finished");
}

void cmd_key_alert(conn_info_t *ci) {
  debug("CMD Key Alert");

  debug("CMD Key Alert Finished");
}
