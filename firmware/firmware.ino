#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RotaryEncoder.h>
#include <pico/mutex.h>

#define _A     "\xB1" // ｱ
#define _I     "\xB2" // ｲ
#define _U     "\xB3" // ｳ
#define _E     "\xB4" // ｴ
#define _O     "\xB5" // ｵ
#define _KA    "\xB6" // ｶ
#define _KI    "\xB7" // ｷ
#define _KU    "\xB8" // ｸ
#define _KE    "\xB9" // ｹ
#define _KO    "\xBA" // ｺ
#define _SA    "\xBB" // ｻ
#define _SI    "\xBC" // ｼ
#define _SU    "\xBD" // ｽ
#define _SE    "\xBE" // ｾ
#define _SO    "\xBF" // ｿ
#define _TA    "\xC0" // ﾀ
#define _TI    "\xC1" // ﾁ
#define _TU    "\xC2" // ﾂ
#define _TE    "\xC3" // ﾃ
#define _TO    "\xC4" // ﾄ
#define _HA    "\xCA" // ﾊ
#define _HI    "\xCB" // ﾋ
#define _HU    "\xCC" // ﾌ
#define _HE    "\xCD" // ﾍ
#define _HO    "\xCE" // ﾎ
#define _MA    "\xCF" // ﾏ
#define _MI    "\xD0" // ﾐ
#define _MU    "\xD1" // ﾑ
#define _ME    "\xD2" // ﾒ
#define _MO    "\xD3" // ﾓ
#define _RA    "\xD7" // ﾗ
#define _RI    "\xD8" // ﾘ
#define _RU    "\xD9" // ﾙ
#define _RE    "\xDA" // ﾚ
#define _RO    "\xDB" // ﾛ
#define _N     "\xDD" // ﾝ
#define _CHO   "\xB0" // ｰ (長音)

#define _DAK   "\xDE" // ﾞ (濁点)
#define _HAN   "\xDF" // ﾟ (半濁点)

// --- 液晶内蔵の便利記号 ---
#define _DEG   "\xDF" // ° (度・半濁点と同じ形状です)
#define _ARR_R "\x7E" // → (右矢印)
#define _ARR_L "\x7F" // ← (左矢印)
#define _DIV   "\xFD" // ÷ (割り算記号)
#define _YEN   "\x5C" // ¥ (フォントにより円マークになります)
#define _MID   "\xA5" // ・（中点）

#define _NA    "\xC5" // ﾅ
#define _NI    "\xC6" // ﾆ
#define _NU    "\xC7" // ﾇ
#define _NE    "\xC8" // ﾈ
#define _NO    "\xC9" // ﾉ

// --- ヤ行 ---
#define _YA    "\xD4" // ﾔ
#define _YU    "\xD5" // ﾕ
#define _YO    "\xD6" // ﾖ

// --- ワ・ヲ ---
#define _WA    "\xDC" // ﾜ
#define _WO    "\xA6" // ｦ

// --- ァィゥェォ（小文字） ---
#define _xA    "\xA7" // ｧ
#define _xI    "\xA8" // ｨ
#define _xU    "\xA9" // ｩ
#define _xE    "\xAA" // ｪ
#define _xO    "\xAB" // ｫ

// --- ャュョッ（小文字） ---
#define _xYA   "\xAC" // ｬ
#define _xYU   "\xAD" // ｭ
#define _xYO   "\xAE" // ｮ
#define _xTU   "\xAF" // ｯ

void menuController();
void lcdHandler();
void kaoHandler(int kaomode);
void showError(int errorcode);
void uvTimer(int runtimer);

auto_init_mutex(displayMutex);

LiquidCrystal_I2C lcd(0x3F, 16, 2);
RotaryEncoder encoder(3, 2, RotaryEncoder::LatchMode::FOUR3);
int lcdmode = 0;
long newpos = 0;
int menuNum = 0;
volatile bool menuWarikomi = 0;
bool isTimerStarted = false;
enum State {
    INITIALIZING,
    MENU_SELECT,
    RUNNING,
    FINISHED,
    ABORTED,
    ERROR
};
volatile State currentState = MENU_SELECT;

void setup() {
    pinMode(4, INPUT); // リセットスイッチ 
    pinMode(5, INPUT); // ロータリーエンコーダー（押し込み）
    pinMode(6, INPUT); // 開扉検知用リミットスイッチ
    pinMode(7, OUTPUT); // UVCランプ用リレー
    pinMode(8, OUTPUT); // ブザー

    Wire.begin();
    mutex_enter_blocking(&displayMutex);
    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(_SI _xYO _KI _KA _TI _xYU _U "...");
    mutex_exit(&displayMutex);
    kaoHandler(3);
    mutex_enter_blocking(&displayMutex);
    lcd.clear();
    mutex_exit(&displayMutex);
    kaoHandler(2);
    lcdHandler();
}

void loop() {

}

void loop1() {
    if(currentState == MENU_SELECT) {
        encoder.tick();
        newpos = encoder.getPosition();
        menuController();
    }
}

void menuController() {
    int timertime = 0;
    static long oldpos = 0;
    const int numOptions = 5; // メニューの選択肢の数 - 1
    if(newpos != oldpos) {
        RotaryEncoder::Direction dir = encoder.getDirection();
        if(dir == RotaryEncoder::Direction::CLOCKWISE) {
            if(menuNum >= numOptions) {
                menuNum = 0;
            } else {
                menuNum++;
            }
        } else if(dir == RotaryEncoder::Direction::COUNTERCLOCKWISE) {
            if(menuNum <= 0) {
                menuNum = numOptions;
            } else {
                menuNum--;
            }
        }
        oldpos = newpos;
    }
    mutex_enter_blocking(&displayMutex);
    lcd.setCursor(0, 1);
    lcd.print(_ME _NI _xYU _CHO);
    lcd.setCursor(0, 0);
    switch(menuNum) {
        case 0:
            timertime = 10;
            lcd.print(_SA _SA _xTU _TO _SI _DAK _xYO _KI _N);
            break;
        case 1:
            timertime = 30;
            lcd.print(_SU _KO _SI _SI _DAK _xYO _KI _N);
            break;
        case 2:
            timertime = 60;
            lcd.print(_HU _TU _U _NI _SI _DAK _xYO _KI _N);
            break;
        case 3:
            timertime = 90;
            lcd.print(_SI _xTU _KA _RI _SHI _DAK _xYO _KI _N);
            break;
        case 4:
            timertime = 120;
            lcd.print(_SI _DAK _xTU _KU _RI _SI _DAK _xYO _KI _N);
            break;
        case 5:
            timertime = 160;
            lcd.print(_SI _DAK _xE _NO _SA _I _TO _DAK _MO _CHO _TO _DAK);
            break;
        default:
            showError(1);
            menuNum = 0;
            break;
    }
    mutex_exit(&displayMutex);
    if(digitalRead(5) == HIGH) {
        currentState = RUNNING;
        uvTimer(timertime);
    }
}

void lcdHandler() {
    int currentKao = 2;
    int volatile emotion = 0;
    
    mutex_enter_blocking(&displayMutex);
    lcd.setCursor(0, 0);
    lcd.print("LCD" _HA _N _TO _DAK _RA _NI _I _KO _U);
    mutex_exit(&displayMutex);
    while(1) {
        if(digitalRead(6) != HIGH && currentState == RUNNING) { //動作中に開扉
            digitalWrite(7, LOW);
            currentState = ABORTED;
            mutex_enter_blocking(&displayMutex);
            lcd.setCursor(0, 0);
            lcd.print(_HI _SI _DAK _xYO _U _TE _I _SI "!!!");
            mutex_exit(&displayMutex);
            kaoHandler(5);
            for(int i = 0; i <= 20; i++) {
                tone(8, 660, 500);
                delay(300);
            }
            while(1) {
                mutex_enter_blocking(&displayMutex);
                lcd.setCursor(0, 0);
                lcd.print(_RI _SE _xTU _TO _HO _DAK _TA _N _WO _NA _KA _DAK _O _SI);
                mutex_exit(&displayMutex);
                kaoHandler(7);
                if(digitalRead(4)) {
                    mutex_enter_blocking(&displayMutex);
                    lcd.setCursor(0, 0);
                    lcd.print(_RI _SE _xTU _TO _SI _TE _I _MA _SU);
                    mutex_exit(&displayMutex);
                    kaoHandler(3);
                    currentState = MENU_SELECT;
                    break;
                }
            }
        } else if(digitalRead(4) == HIGH && currentState == RUNNING) {
            digitalWrite(7, LOW);
            currentState = ABORTED;
            mutex_enter_blocking(&displayMutex);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print(_TE _I _SI _SI _MA _SI _TA);
            mutex_exit(&displayMutex);
            kaoHandler(4);
            tone(8, 660, 1000);
        }
        if(emotion == 0) { // 普通
            if((millis() / 500) % 10 == 0) {
                currentKao = 0;
            } else {
                currentKao = 2;
            }
        } else if(emotion == 1) { // 混乱
            currentKao = 5;
        }
        kaoHandler(currentKao);
        delay(50);
    }
}

void kaoHandler(int kaomode) {
    mutex_enter_blocking(&displayMutex);
    switch(kaomode) {
        case 0: // (-_-)
            lcd.setCursor(10, 1);
            lcd.print("(-_-)");
            break;
        case 1: // (・_・)
            lcd.setCursor(10, 1);
            lcd.print("("  _MID "_" _MID ")");
            break;
        case 2: // (^_^)
            lcd.setCursor(10, 1);
            lcd.print("(^_^)");
            break;
        case 3: // Zzz (-_-)
            lcd.setCursor(10, 1);
            lcd.print("(-_-)");
            for(int j = 0; j <= 2; j++) {
                for(int i = 0; i <= 3; i++) {
                    lcd.setCursor(6, 1);
                    if(i == 0) {
                        lcd.print("   ");
                    } else if(i == 1) {
                        lcd.print("  z");
                    } else if(i == 2) {
                        lcd.print(" zz");
                    } else if(i == 3) {
                        lcd.print("Zzz");
                    }
                    delay(500);
                }
            }
            break;
        case 4: // (>_<)
            lcd.setCursor(10, 1);
            lcd.print("(>_<)");
            break;
        case 5: // (@_@)
            lcd.setCursor(10, 1);
            lcd.print("(@_@)");
            break;
        case 6: // (#_#)
            lcd.setCursor(10, 1);
            lcd.print("(#_#)");
            break;
        case 7: // (;_;)
            lcd.setCursor(10, 1);
            lcd.print("(;_;)");
            break;
        default:
            lcd.setCursor(4, 1);
            lcd.print("Error (#_#)");
            break;
    }
    mutex_exit(&displayMutex);
}

void showError(int errorcode) {
    switch(errorcode) {
        case 0:
            mutex_enter_blocking(&displayMutex);
            lcd.clear();
            mutex_exit(&displayMutex);
            while(1) {
                kaoHandler(6);
                mutex_enter_blocking(&displayMutex);
                lcd.setCursor(0, 0);
                lcd.print(_E _RA _CHO _KA _DAK _HA _xTU _SE _I _SI _MA _SI _TA);
                delay(2000);
                lcd.clear();
                mutex_exit(&displayMutex);
                kaoHandler(5);
                mutex_enter_blocking(&displayMutex);
                lcd.setCursor(0, 0);
                lcd.print(_SA _I _KI _TO _DAK _U _SI _TE _KU _TA _DAK _SA _I);
                delay(2000);
                lcd.clear();
                mutex_exit(&displayMutex);
            }
            break;
        case 1:
            mutex_enter_blocking(&displayMutex);
            lcd.clear();
            mutex_exit(&displayMutex);
            kaoHandler(6);
            mutex_enter_blocking(&displayMutex);
            lcd.print(_E _RA _CHO _KA _DAK _HA _xTU _SE _I _SI _MA _SI _TA);
            mutex_exit(&displayMutex);
            delay(5000);
            break;
        default:
            break;
    }
}

void uvTimer(int runtimer) {
    if (digitalRead(6) != HIGH) {
        mutex_enter_blocking(&displayMutex);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(_TO _HI _DAK _RA _KA _DAK _A _I _TE _I _MA _SU);
        mutex_exit(&displayMutex);
        tone(8, 440, 500);
        delay(2000);
        mutex_enter_blocking(&displayMutex);
        lcd.clear();
        mutex_exit(&displayMutex);
        currentState = MENU_SELECT;
        return;
    }

    mutex_enter_blocking(&displayMutex);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(_SI _DAK _xYO _KI _N _TI _xYU _U "...");
    mutex_exit(&displayMutex);

    digitalWrite(7, HIGH);

    for (int t = runtimer; t > 0; t--) {
        // ABORTEDになった場合の緊急脱出
        if (currentState == ABORTED) {
            digitalWrite(7, LOW);
            return; 
        }

        mutex_enter_blocking(&displayMutex);
        lcd.setCursor(0, 1);
        lcd.print(_NO _KO _RI " ");

        if (t < 100) lcd.print(" ");
        if (t < 10)  lcd.print(" ");
        lcd.print(t);
        lcd.print(" Sec ");
        mutex_exit(&displayMutex);

        delay(1000);
    }

    digitalWrite(7, LOW);
    currentState = FINISHED;

    mutex_enter_blocking(&displayMutex);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(_KA _N _RI _xYO _U _SI _MA _SI _TA);
    mutex_exit(&displayMutex);
    
    tone(8, 1000, 200); delay(300);
    tone(8, 1000, 200); delay(300);
    tone(8, 1000, 800); delay(1000);

    delay(2000);
    
    currentState = MENU_SELECT;
    mutex_enter_blocking(&displayMutex);
    lcd.clear();
    mutex_exit(&displayMutex);
}