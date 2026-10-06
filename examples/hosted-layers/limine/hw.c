/* COM1, the clocks and QEMU's exit device: see hw.h. */
#include "hw.h"

/* ---- COM1 (a 16550 UART at I/O port 0x3f8) ---- */
enum {
  com1 = 0x3f8,
  uart_data = 0,          /* with DLAB: divisor low byte */
  uart_interrupts = 1,    /* with DLAB: divisor high byte */
  uart_fifo = 2,
  uart_line_control = 3,
  uart_modem_control = 4,
  uart_line_status = 5,
};

void serial_init(void) {
  outb(com1 + uart_interrupts, 0x00);    /* no interrupts: the kernel polls */
  outb(com1 + uart_line_control, 0x80);  /* DLAB on: set the baud rate divisor */
  outb(com1 + uart_data, 0x01);          /* 115200 / 1 */
  outb(com1 + uart_interrupts, 0x00);
  outb(com1 + uart_line_control, 0x03);  /* 8 bits, no parity, one stop bit; DLAB off */
  outb(com1 + uart_fifo, 0xc7);          /* FIFOs on and cleared */
  outb(com1 + uart_modem_control, 0x03); /* DTR, RTS */
}

static void serial_put(char c) {
  while ((inb(com1 + uart_line_status) & 0x20) == 0) /* transmitter holding register empty */
    ;
  outb(com1 + uart_data, (uint8_t)c);
}

void serial_write(const char* s, size_t n) {
  for (size_t i = 0; i < n; ++i) {
    if (s[i] == '\n')
      serial_put('\r');
    serial_put(s[i]);
  }
}

void serial_puts(const char* s) {
  size_t n = 0;
  while (s[n] != '\0')
    ++n;
  serial_write(s, n);
}

int serial_read(char* c) {
  while ((inb(com1 + uart_line_status) & 0x01) == 0) /* data ready */
    ;
  *c = (char)inb(com1 + uart_data);
  return 0;
}

/* ---- clocks ---- */
static uint64_t tsc_hz;
static uint64_t tsc_at_boot;
static int64_t realtime_at_boot_ns;

/* The PIT's channel 2 counts down at 1193182 Hz; with its gate on and the speaker off (port 0x61
   bits 0 and 1) and in mode 0, its output (port 0x61 bit 5) goes high when the count ends. The
   TSC ticks counted meanwhile give its frequency. */
static uint64_t calibrate_tsc(void) {
  const uint32_t pit_hz = 1193182;
  const uint16_t count = 23864; /* 20 ms */
  const uint8_t saved = inb(0x61);
  outb(0x61, (uint8_t)((saved & ~0x02) | 0x01));
  outb(0x43, 0xb0); /* channel 2, low then high byte, mode 0, binary */
  outb(0x42, (uint8_t)(count & 0xff));
  outb(0x42, (uint8_t)(count >> 8));
  const uint64_t t0 = rdtsc();
  uint64_t spins = 0;
  while ((inb(0x61) & 0x20) == 0)
    if (++spins == 100000000) /* no PIT: assume 1 GHz */
      return 1000000000;
  const uint64_t t1 = rdtsc();
  outb(0x61, saved);
  return (t1 - t0) * pit_hz / count;
}

/* The CMOS real-time clock (ports 0x70/0x71): seconds 0x00, minutes 0x02, hours 0x04, day 0x07,
   month 0x08, year 0x09, century 0x32 (QEMU has it); status A 0x0a (bit 7: update in progress),
   status B 0x0b (bit 2: binary, else BCD; bit 1: 24-hour). QEMU's runs in UTC by default. */
static uint8_t cmos(uint8_t reg) {
  outb(0x70, reg);
  return inb(0x71);
}

static int is_leap(int64_t y) { return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0; }

static int64_t read_rtc_unix_seconds(void) {
  uint8_t r[7], again[7];
  const uint8_t regs[7] = {0x00, 0x02, 0x04, 0x07, 0x08, 0x09, 0x32};
  /* Read until two reads agree, each outside an update. */
  for (int tries = 0; tries < 1000; ++tries) {
    while (cmos(0x0a) & 0x80)
      ;
    for (int i = 0; i < 7; ++i)
      r[i] = cmos(regs[i]);
    while (cmos(0x0a) & 0x80)
      ;
    for (int i = 0; i < 7; ++i)
      again[i] = cmos(regs[i]);
    int same = 1;
    for (int i = 0; i < 7; ++i)
      same &= r[i] == again[i];
    if (same)
      break;
  }
  const uint8_t status_b = cmos(0x0b);
  const int pm = (r[2] & 0x80) != 0;
  r[2] &= 0x7f;
  if ((status_b & 0x04) == 0) /* BCD */
    for (int i = 0; i < 7; ++i)
      r[i] = (uint8_t)((r[i] >> 4) * 10 + (r[i] & 0x0f));
  if ((status_b & 0x02) == 0) /* 12-hour */
    r[2] = (uint8_t)(r[2] % 12 + (pm ? 12 : 0));
  const int64_t century = r[6] >= 19 && r[6] <= 30 ? r[6] : 20;
  const int64_t year = century * 100 + r[5];
  static const uint8_t month_days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  int64_t days = 0;
  for (int64_t y = 1970; y < year; ++y)
    days += is_leap(y) ? 366 : 365;
  for (int m = 1; m < r[4] && m <= 12; ++m)
    days += month_days[m - 1] + (m == 2 && is_leap(year));
  days += r[3] - 1;
  return ((days * 24 + r[2]) * 60 + r[1]) * 60 + r[0];
}

void clocks_init(void) {
  tsc_hz = calibrate_tsc();
  tsc_at_boot = rdtsc();
  realtime_at_boot_ns = read_rtc_unix_seconds() * 1000000000;
}

uint64_t clock_tsc_hz(void) { return tsc_hz; }

int64_t clock_monotonic_ns(void) {
  const unsigned __int128 ticks = rdtsc() - tsc_at_boot;
  return (int64_t)(ticks * 1000000000u / tsc_hz);
}

int64_t clock_realtime_ns(void) { return realtime_at_boot_ns + clock_monotonic_ns(); }

/* ---- QEMU's exit device ---- */
_Noreturn void qemu_exit(uint32_t code) {
  outl(0xf4, code);
  for (;;)
    __asm__ volatile("cli; hlt");
}
