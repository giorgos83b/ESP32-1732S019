#include <Arduino.h>
#include <TFT_eSPI.h>

// ESP32-1732S019: ST7789, landscape 320 x 170.
// Uses the TFT_eSPI configuration already provided by your project.
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite canvas = TFT_eSprite(&tft);

static const int SCREEN_W = 320;
static const int SCREEN_H = 170;
static const int QUOTE_COUNT = 100;

static const uint32_t QUOTE_DURATION_MS = 8000; // Time each quote stays visible
static const uint32_t FADE_DURATION_MS = 350;   // Fade between quotes
static const uint32_t FRAME_INTERVAL_MS = 35;

static const uint16_t BG       = 0x0841; // deep blue-gray
static const uint16_t ACCENT   = 0x4FEA; // mint
static const uint16_t WHITE    = 0xFFFF;
static const uint16_t MUTED    = 0x9CF3;
static const uint16_t DIVIDER  = 0x2965;

const char* quotes[QUOTE_COUNT] = {
  "Small steps every day create big change.",
  "Progress matters more than perfection.",
  "Start before you feel completely ready.",
  "Consistency turns effort into achievement.",
  "You can do hard things one step at a time.",
  "Your future is shaped by what you do today.",
  "Focus on the next useful step.",
  "Keep going; growth takes time.",
  "Choose courage over comfort.",
  "Make room for the person you are becoming.",
  "Every setback can teach you something.",
  "Your effort is never wasted when you learn.",
  "Be patient with the process.",
  "Begin with what you have.",
  "Action makes the path clearer.",
  "Build habits that support your dreams.",
  "Small wins build lasting confidence.",
  "Show up for yourself today.",
  "Your pace is still progress.",
  "Let discipline carry you when motivation fades.",
  "Turn challenges into practice.",
  "Do the work; trust the growth.",
  "One focused hour can change your day.",
  "Protect your attention; it shapes your life.",
  "Make the next choice a good one.",
  "Confidence grows through action.",
  "Learn, adjust, and try again.",
  "Bravery can be quiet and steady.",
  "You are allowed to start over.",
  "Keep your promises to yourself.",
  "Hard days do not erase your progress.",
  "Rest is part of moving forward.",
  "Be curious instead of fearful.",
  "Make progress, then make it better.",
  "Your limits can move with practice.",
  "Do not confuse slow with stuck.",
  "Let purpose guide your priorities.",
  "Finish one thing before starting five.",
  "Make today useful, not perfect.",
  "Give your best to what matters most.",
  "Small routines create strong foundations.",
  "Choose the response you will respect later.",
  "Stay teachable and keep growing.",
  "Your next chapter starts with one decision.",
  "Turn intention into a small action.",
  "Focus on effort you can control.",
  "Give yourself credit for showing up.",
  "Better questions lead to better paths.",
  "Be consistent, especially when it is quiet.",
  "Build a life you do not need to escape.",
  "Your attention is a valuable resource.",
  "Make courage a daily practice.",
  "Every expert once began as a beginner.",
  "Keep learning beyond your comfort zone.",
  "Let mistakes become useful information.",
  "Be proud of honest effort.",
  "Take the first step; momentum can follow.",
  "Do not wait for perfect conditions.",
  "Choose growth over comparison.",
  "Measure yourself by your own progress.",
  "Give difficult things another thoughtful try.",
  "Make your environment support your goals.",
  "Your habits vote for your future.",
  "Let patience work alongside ambition.",
  "Be the calm in your own storm.",
  "Keep your standards high and your self-talk kind.",
  "Do something today your future self will thank you for.",
  "Make space for focus and recovery.",
  "Turn worry into one practical step.",
  "Your best can look different each day.",
  "Stay steady when results take time.",
  "Progress begins where excuses end.",
  "Use what you know; learn what you need.",
  "Do not let one bad moment define the day.",
  "Bring intention to ordinary moments.",
  "Keep the goal clear and the next step small.",
  "Be willing to be a beginner again.",
  "Choose persistence over instant comfort.",
  "Make your actions match your values.",
  "Strong roots grow out of unseen work.",
  "Let your curiosity outlast your doubt.",
  "Celebrate progress without stopping there.",
  "Move forward with what you learned.",
  "Spend energy where you have influence.",
  "Make kindness part of your strength.",
  "Keep your mind open and your feet moving.",
  "Your story is still being written.",
  "Replace self-doubt with a small experiment.",
  "Do not rush what deserves care.",
  "Build momentum one promise at a time.",
  "Let today be a fresh attempt.",
  "Stay focused on what you are building.",
  "Make the difficult choice that helps you grow.",
  "Your effort today is an investment.",
  "Be patient; meaningful change compounds.",
  "Take the lesson and leave the shame.",
  "Small acts of courage add up.",
  "Keep becoming, one day at a time.",
  "Begin again, as often as needed.",
  "Make this moment count."
};

int order[QUOTE_COUNT];
int currentPosition = 0;
int currentQuote = 0;
uint32_t quoteStartedAt = 0;
uint32_t lastFrameAt = 0;

static const int FONT_ID = 2;
static const int MAX_LINES = 7;
String wrappedLines[MAX_LINES];
int lineCount = 0;

uint16_t scaleColor(uint16_t color, uint8_t brightness) {
  uint8_t r = ((color >> 11) & 0x1F) * brightness / 255;
  uint8_t g = ((color >> 5) & 0x3F) * brightness / 255;
  uint8_t b = (color & 0x1F) * brightness / 255;
  return (r << 11) | (g << 5) | b;
}

void shuffleQuotes() {
  for (int i = 0; i < QUOTE_COUNT; i++) order[i] = i;

  // Fisher-Yates shuffle.
  for (int i = QUOTE_COUNT - 1; i > 0; i--) {
    int j = random(i + 1);
    int temp = order[i];
    order[i] = order[j];
    order[j] = temp;
  }

  // At a cycle boundary, avoid repeating the previous cycle's last quote.
  if (QUOTE_COUNT > 1 && order[0] == currentQuote) {
    int temp = order[0];
    order[0] = order[1];
    order[1] = temp;
  }

  currentPosition = 0;
}

void wrapCurrentQuote() {
  lineCount = 0;
  String remaining = quotes[currentQuote];
  remaining.trim();

  const int maxWidth = SCREEN_W - 44;

  while (remaining.length() > 0 && lineCount < MAX_LINES) {
    int split = remaining.length();

    while (split > 0) {
      String candidate = remaining.substring(0, split);
      candidate.trim();

      if (canvas.textWidth(candidate, FONT_ID) <= maxWidth) break;

      int space = remaining.lastIndexOf(' ', split - 1);
      if (space > 0) split = space;
      else split--;
    }

    if (split <= 0) split = 1;

    String line = remaining.substring(0, split);
    line.trim();
    if (line.length() > 0) wrappedLines[lineCount++] = line;

    remaining = remaining.substring(split);
    remaining.trim();
  }
}

void drawQuote(uint8_t brightness) {
  canvas.fillSprite(BG);

  // Header
  canvas.setTextColor(scaleColor(ACCENT, brightness), BG);
  canvas.drawString("A MOMENT OF MOTIVATION", 10, 7, FONT_ID);
  canvas.drawFastHLine(10, 27, SCREEN_W - 20,
                       scaleColor(DIVIDER, brightness));

  // Center the wrapped quote in the available area.
  const int lineHeight = 20;
  const int quoteAreaTop = 34;
  const int quoteAreaBottom = 137;
  int blockHeight = lineCount * lineHeight;
  int y = quoteAreaTop + ((quoteAreaBottom - quoteAreaTop) - blockHeight) / 2;

  for (int i = 0; i < lineCount; i++) {
    canvas.setTextColor(scaleColor(WHITE, brightness), BG);
    canvas.drawCentreString(wrappedLines[i], SCREEN_W / 2,
                            y + i * lineHeight, FONT_ID);
  }

  // Footer: quote number and progress bar.
  canvas.drawFastHLine(10, 145, SCREEN_W - 20,
                       scaleColor(DIVIDER, brightness));
  canvas.setTextColor(scaleColor(MUTED, brightness), BG);
  char label[24];
  snprintf(label, sizeof(label), "QUOTE %03d / %03d",
           currentQuote + 1, QUOTE_COUNT);
  canvas.drawString(label, 10, 151, FONT_ID);

  uint32_t elapsed = millis() - quoteStartedAt;
  uint32_t remaining = elapsed >= QUOTE_DURATION_MS
                         ? 0 : QUOTE_DURATION_MS - elapsed;
  char seconds[12];
  snprintf(seconds, sizeof(seconds), "%lus",
           (unsigned long)((remaining + 999) / 1000));
  canvas.drawRightString(seconds, SCREEN_W - 10, 151, FONT_ID);

  int barX = 10, barY = 166, barW = SCREEN_W - 20;
  canvas.drawFastHLine(barX, barY, barW, scaleColor(DIVIDER, brightness));
  int filled = (elapsed >= QUOTE_DURATION_MS) ? barW :
               (int)((uint64_t)barW * elapsed / QUOTE_DURATION_MS);
  if (filled > 0) {
    canvas.drawFastHLine(barX, barY, filled,
                         scaleColor(ACCENT, brightness));
  }

  canvas.pushSprite(0, 0);
}

void showNextQuote() {
  currentPosition++;

  if (currentPosition >= QUOTE_COUNT) {
    shuffleQuotes();
  }

  currentQuote = order[currentPosition];
  wrapCurrentQuote();
  quoteStartedAt = millis();

  // Fade in the new quote.
  for (int brightness = 0; brightness <= 255; brightness += 51) {
    drawQuote((uint8_t)brightness);
    delay(12);
  }
  drawQuote(255);
}

void setup() {
  Serial.begin(115200);
  randomSeed((uint32_t)esp_random());

  tft.init();
  tft.setRotation(1); // Landscape: 320 x 170
  tft.fillScreen(BG);

  canvas.setColorDepth(16);
  if (canvas.createSprite(SCREEN_W, SCREEN_H) == nullptr) {
    Serial.println("ERROR: Could not allocate display sprite.");
    while (true) delay(1000);
  }

  canvas.setTextFont(FONT_ID);
  canvas.setTextSize(1);
  canvas.setTextWrap(false, false);

  shuffleQuotes();
  currentQuote = order[currentPosition];
  wrapCurrentQuote();
  quoteStartedAt = millis();

  drawQuote(255);
  Serial.println("Motivational quote display ready: 100 shuffled quotes.");
}

void loop() {
  uint32_t now = millis();

  if (now - quoteStartedAt >= QUOTE_DURATION_MS) {
    // Fade out the current quote, then advance and fade in the next.
    for (int brightness = 255; brightness >= 0; brightness -= 51) {
      drawQuote((uint8_t)brightness);
      delay(12);
    }
    showNextQuote();
    lastFrameAt = millis();
    return;
  }

  if (now - lastFrameAt >= FRAME_INTERVAL_MS) {
    lastFrameAt = now;
    drawQuote(255);
  }
}
