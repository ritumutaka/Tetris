/*
コンパイル例：gcc -o Tetris Tetris.c
実行例：./Tetris

＜仕様/操作方法＞
上 なし
下 S
左 D
右 A
回転　スペース

Mac向けターミナル用テトリスです。
Windowsで機能するかは分かりません。

なるべく多く同時消しすると高得点になります。
また、経過時間が長いほどもらえる得点が高くなります。

*/

#include <stdio.h>
#include <stdlib.h>   //画面初期化用
#include <string.h>
#include <time.h>     //1秒ごとの処理用、ランダム関数初期化用


//画面入力待ち用-------------------------------------------------------------------------------------------
#include <termios.h>  //入力待ち用
#include <unistd.h>   //入力待ち用
int mygetch( ) {
  struct termios oldt,
                 newt;
  int            ch;
  tcgetattr( STDIN_FILENO, &oldt );
  newt = oldt;
  newt.c_lflag &= ~( ICANON | ECHO );
  tcsetattr( STDIN_FILENO, TCSANOW, &newt );
  ch = getchar();
  tcsetattr( STDIN_FILENO, TCSANOW, &oldt );
  return ch;
}

//kbhit()の代わり------------------------------------------------------------------------------------------------------
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
int kbhit(void) {
  struct termios oldt, newt;
  int ch;
  int oldf;

  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

  ch = getchar();

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);

  if (ch != EOF) {
    ungetc(ch, stdin);
    return 1;
  }

  return 0;
}

/*------------------------------------------------------------------------------------------------------------*/
#define FIELD_WIDTH 12
#define FIELD_HEIGHT 22

char field[FIELD_HEIGHT][FIELD_WIDTH];
char displayBuffer[FIELD_HEIGHT][FIELD_WIDTH]; //フィールドとミノを合成したもの

enum {  //ミノの種類
    MINO_TYPE_I,
    MINO_TYPE_O,
    MINO_TYPE_S,
    MINO_TYPE_Z,
    MINO_TYPE_J,
    MINO_TYPE_L,
    MINO_TYPE_T,
    MINO_TYPE_MAX
};

enum {  //ミノのアングル
    MINO_ANGLE_0,
    MINO_ANGLE_90,
    MINO_ANGLE_180,
    MINO_ANGLE_270,
    MINO_ANGLE_MAX
};

#define MINO_WIDTH 4
#define MINO_HEIGHT 4
char minoShapes[MINO_TYPE_MAX][MINO_ANGLE_MAX][MINO_HEIGHT][MINO_WIDTH] = { //全てのミノが詰まった配列
    // MINO_TYPE_I,
    {
        // MINO_ANGLE_0
        {
            0,1,0,0,
            0,1,0,0,
            0,1,0,0,
            0,1,0,0,
        },
        // MINO_ANGLE_90
        {
            0,0,0,0,
            0,0,0,0,
            1,1,1,1,
            0,0,0,0,
        },
        // MINO_ANGLE_180
        {
            0,0,1,0,
            0,0,1,0,
            0,0,1,0,
            0,0,1,0,
        },
        // MINO_ANGLE_270
        {
            0,0,0,0,
            1,1,1,1,
            0,0,0,0,
            0,0,0,0,
        },
    },
    // MINO_TYPE_O,
    {
        // MINO_ANGLE_0
        {
            0,0,0,0,
            0,1,1,0,
            0,1,1,0,
            0,0,0,0,
        },
        // MINO_ANGLE_90
        {
            0,0,0,0,
            0,1,1,0,
            0,1,1,0,
            0,0,0,0,
        },
        // MINO_ANGLE_180
        {
            0,0,0,0,
            0,1,1,0,
            0,1,1,0,
            0,0,0,0,
        },
        // MINO_ANGLE_270
        {
            0,0,0,0,
            0,1,1,0,
            0,1,1,0,
            0,0,0,0,
        },
    },
    // MINO_TYPE_S,
    {
        // MINO_ANGLE_0
        {
            0,0,0,0,
            0,1,1,0,
            1,1,0,0,
            0,0,0,0,
        },
        // MINO_ANGLE_90
        {
            0,0,0,0,
            0,1,0,0,
            0,1,1,0,
            0,0,1,0,
        },
        // MINO_ANGLE_180
        {
            0,0,0,0,
            0,0,1,1,
            0,1,1,0,
            0,0,0,0,
        },
        // MINO_ANGLE_270
        {
            0,1,0,0,
            0,1,1,0,
            0,0,1,0,
            0,0,0,0,
        },
    },
    // MINO_TYPE_Z,
    {
        // MINO_ANGLE_0
        {
            0,0,0,0,
            1,1,0,0,
            0,1,1,0,
            0,0,0,0,
        },
        // MINO_ANGLE_90
        {
            0,0,0,0,
            0,0,1,0,
            0,1,1,0,
            0,1,0,0,
        },
        // MINO_ANGLE_180
        {
            0,0,0,0,
            0,1,1,0,
            0,0,1,1,
            0,0,0,0,
        },
        // MINO_ANGLE_270
        {
            0,0,1,0,
            0,1,1,0,
            0,1,0,0,
            0,0,0,0,
        },
    },
    // MINO_TYPE_J,
    {
        // MINO_ANGLE_0
        {
            0,0,1,0,
            0,0,1,0,
            0,1,1,0,
            0,0,0,0,
        },
        // MINO_ANGLE_90
        {
            0,0,0,0,
            1,1,1,0,
            0,0,1,0,
            0,0,0,0,
        },
        // MINO_ANGLE_180
        {
            0,0,0,0,
            0,1,1,0,
            0,1,0,0,
            0,1,0,0,
        },
        // MINO_ANGLE_270
        {
            0,0,0,0,
            0,1,0,0,
            0,1,1,1,
            0,0,0,0,
        },
    },
    // MINO_TYPE_L,
    {
        // MINO_ANGLE_0
        {
            0,1,0,0,
            0,1,0,0,
            0,1,1,0,
            0,0,0,0,
        },
        // MINO_ANGLE_90
        {
            0,0,0,0,
            0,0,1,0,
            1,1,1,0,
            0,0,0,0,
        },
        // MINO_ANGLE_180
        {
            0,0,0,0,
            0,1,1,0,
            0,0,1,0,
            0,0,1,0,
        },
        // MINO_ANGLE_270
        {
            0,0,0,0,
            0,1,1,1,
            0,1,0,0,
            0,0,0,0,
        },
    },
    // MINO_TYPE_T,
    {
        // MINO_ANGLE_0
        {
            0,0,0,0,
            0,1,0,0,
            1,1,1,0,
            0,0,0,0,
        },
        // MINO_ANGLE_90
        {
            0,0,0,0,
            0,0,1,0,
            0,1,1,0,
            0,0,1,0,
        },
        // MINO_ANGLE_180
        {
            0,0,0,0,
            0,1,1,1,
            0,0,1,0,
            0,0,0,0,
        },
        // MINO_ANGLE_270
        {
            0,1,0,0,
            0,1,1,0,
            0,1,0,0,
            0,0,0,0,
        },
    }
};

int minoType , minoAngle;
int minoX , minoY;
int endFlg = 0;
int totalScore = 0;


/*------------------------------------------------------------------------------------------------------------*/
void display(int nowTimeCount) {    //描画用
    memcpy(displayBuffer, field, sizeof(field));
        
        for (int y = 0; y < MINO_HEIGHT; y++) {
            for (int x = 0; x < MINO_WIDTH; x++) {
                displayBuffer[minoY + y][minoX + x] |= minoShapes[minoType][minoAngle][y][x];
            }
        }

        system("clear");
        //合成されたフィールドを描画
        for (int y = 0; y < FIELD_HEIGHT; y++) {
            for (int x = 0; x < FIELD_WIDTH; x++) {
                printf(displayBuffer[y][x]?"□":"  ");
            }
            printf("\n");
        }

        //スコア表示
        printf("スコア：%d\n", totalScore);
        printf("タイム：%d\n", nowTimeCount);
}

int isHit(int _minoX, int _minoY, int _minoType, int _minoAngle) {      //壁にぶつかる判定用
    for (int y = 0; y < MINO_HEIGHT; y++) {
        for (int x = 0; x < MINO_WIDTH; x++) {
            if (minoShapes[_minoType][_minoAngle][y][x]
                && field[_minoY + y][_minoX + x]) {
                return 1;
            }
        }
    }
    return 0;
}

void resetMino() {  //ミノを新しくする
    minoX = 4;
    minoY = -1;
    minoType = rand() % MINO_TYPE_MAX;
    minoAngle = rand() % MINO_ANGLE_MAX;

    if (isHit(minoX, minoY, minoType, minoAngle)) {  //ゲームオーバーにならないか判定
        endFlg = 1;
    }
}


/*------------------------------------------------------------------------------------------------------------*/
int main(void) {
    
    memset(field, 0, sizeof(field));    //フィールドを作成
    for (int y = 0; y < FIELD_HEIGHT; y++) {
        field[y][0] = field[y][FIELD_WIDTH-1] = 1;
    }
    for (int x = 0; x < FIELD_WIDTH; x++) {
        field[FIELD_HEIGHT-1][x] = 1;
    }

    int nowTimeCount = 0;
    time_t t = time(NULL);
    srand((unsigned int)time(NULL));    //ランダム関数を現在時刻で初期化
    resetMino();
    while(1) {  //メインループ
        if (kbhit()) {  //キーボードが押されたら、キーに応じた処理後に再描画
            switch (mygetch()) {
            case 'd':
                if(!isHit(minoX+1, minoY, minoType, minoAngle))
                    minoX++;
                break;
            case 'a': 
                if(!isHit(minoX-1, minoY, minoType, minoAngle))
                    minoX--;
                break;
            case 's': 
                if(!isHit(minoX, minoY+1, minoType, minoAngle))
                    minoY++;
                break;
            case 0x20:
                if(!isHit(minoX, minoY, minoType, minoAngle+1))
                    minoAngle = (minoAngle +1) % MINO_ANGLE_MAX;
                break;
            default:
                break;
            }
            display(nowTimeCount);  //再描画
        }

        if ( t != time(NULL)) {
            nowTimeCount++;  //経過時間のカウント
            t = time(NULL);
            if (isHit(minoX, minoY+1, minoType, minoAngle)) {   //下に何かあるかチェック
                for (int y = 0; y < MINO_HEIGHT; y++)
                    for (int x = 0; x < MINO_WIDTH; x++)
                        field[minoY + y][minoX + x] |= minoShapes[minoType][minoAngle][y][x];   //何かにぶつかったら固定（フィールドと一体化)
                    
                int totalClearLine = 0;
                for (int y = 0; y < FIELD_HEIGHT - 1; y++) {    //固定されたら、１行揃ってるか確認
                    int lineFill = 1;   //揃っていると仮定する
                    for (int x = 0; x < FIELD_WIDTH - 1; x++) {
                        if (!field[y][x]) { //もし０があれば揃っていない
                            lineFill = 0;
                        }
                    }
                    
                    if (lineFill) { //もし揃っていれば
                        for (int a = y; a > 0; a--) {
                            memcpy(field[a], field[a-1], FIELD_WIDTH);  //上の行を１段下へコピー
                        }
                        totalClearLine++;
                        totalScore += (1000 + nowTimeCount * 5) * totalClearLine;    //スコア計算して加算
                    }
                }
                
                
                resetMino();
            } else {
                minoY++;
            }
            display(nowTimeCount);
        }
        if (endFlg) {
            goto OUT;
        }
    }
    OUT:
        printf("GAME OVER!\n");
}