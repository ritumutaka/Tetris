#include <stdio.h>
#include <stdlib.h>   //画面初期化用
#include <time.h>     //ランダム関数初期化用
#include <string.h>   //memcpy用

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

#define FIELD_WIDTH 32   //フィールドの広さ
#define FIELD_HEIGHT 32

#define AREA_MAX 64
#define AREA_SIZE_MIN 8   //エリアの最小値（部屋の大きさに関係）

#define SCREEN_RANGE 16    //見える範囲の大きさ

enum {  //セルの中身のタイプを定義
  CELL_TYPE_NONE,
  CELL_TYPE_WALL,
  CELL_TYPE_STAIRS,
  CELL_TYPE_AMULET,
  CELL_TYPE_PLAYER,
  CELL_TYPE_ENEMY,
};

char cellAA[][2 + 2] = {  //セル中身に対応した表記  注）\0を入れないと全て表示される
  "  \0",   //CELL_TYPE_NONE
  "團\0",   //CELL_TYPE_WALL
  "％\0",   //CELL_TYPE_STAIRS
  "宝\0",   //CELL_TYPE_AMULET
  "＠\0",   //CELL_TYPE_PLAYER
  "鬼\0",   //CELL_TYPE_ENEMY
};

//部屋定義---------------------------------------------------------------------------------------------------
typedef struct {
  int x, y, w, h;
} ROOM;

//エリア区切り用定義---------------------------------------------------------------------------------------------------
typedef struct {
  int x, y, w, h;
  ROOM room;
} AREA;

AREA areas[AREA_MAX];
int areaCount;

int field[FIELD_HEIGHT][FIELD_WIDTH];
int buffer[FIELD_HEIGHT][FIELD_WIDTH];

typedef struct {
  int x, y;
  int hp, maxHp;
  int attack;
} CHARACTER;

int damage; //ダメージ計算用

CHARACTER player;
CHARACTER enemy;
int floorNum = 1;

int distance;   //テスト用グローバル変数化~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


void displayArea() {  //buffer[]にエリアを書き込んで表示
  int buffer[FIELD_HEIGHT][FIELD_WIDTH] = {};
  for (int i = 0; i < areaCount; i++) //エリア番号をbuffer[]に書き込む
    for (int y = areas[i].y; y < areas[i].y + areas[i].h; y++)
      for (int x = areas[i].x; x < areas[i].x + areas[i].w; x++)
        buffer[y][x] = i;
  
  system("clear");
  for (int y = 0; y < FIELD_HEIGHT; y++) {   //全てのマスを表示していく
    for (int x = 0; x < FIELD_WIDTH; x++) {
      char str[] = "０";      //全角で表示するための処理
      str[2] += buffer[y][x] % 10;
      printf("%s", str);
    }
    printf(": %d\n", y);
  }
}

void splitArea(int _areaIdx) {  //エリアを区切る
  int newAreaIdx = areaCount;
  int w = areas[_areaIdx].w;
  int h = areas[_areaIdx].h;

  if (rand()%2) {
    {   //縦に分割
      areas[_areaIdx].w /= 2;
      
      areas[newAreaIdx].x = areas[_areaIdx].x + areas[_areaIdx].w;
      areas[newAreaIdx].y = areas[_areaIdx].y;
      areas[newAreaIdx].w = w - areas[_areaIdx].w;
      areas[newAreaIdx].h = areas[_areaIdx].h;
    }
  } else {
    {   //横に分割
      areas[_areaIdx].h /= 2;

      areas[newAreaIdx].x = areas[_areaIdx].x;
      areas[newAreaIdx].y = areas[_areaIdx].y + areas[_areaIdx].h;
      areas[newAreaIdx].w = areas[_areaIdx].w;
      areas[newAreaIdx].h = h - areas[_areaIdx].h;
    }
  }

  if ((areas[_areaIdx].w < AREA_SIZE_MIN) || (areas[_areaIdx].h < AREA_SIZE_MIN)   //分割した時に最小サイズを下回ったらキャンセル
      || (areas[newAreaIdx].w < AREA_SIZE_MIN) || (areas[newAreaIdx].h < AREA_SIZE_MIN)) {
      areas[_areaIdx].w = w;
      areas[_areaIdx].h = h;
      return;
    }

  areaCount++;

  splitArea(_areaIdx);
  splitArea(newAreaIdx);
}

void setRandPosition(int *_pX, int *_pY) {  //ランダムな座標を取得　（引数はアドレス）
  int areaIdx = rand() % areaCount;
  *_pX = areas[areaIdx].room.x + rand() % areas[areaIdx].room.w;
  *_pY = areas[areaIdx].room.y + rand() % areas[areaIdx].room.h;
}

void generateField() {  //フィールド（エリア分割済み）の作成
  areaCount = 0;  //0番目のエリアを作成
  areas[0].x = 
  areas[0].y = 0;
  areas[0].w = FIELD_WIDTH;
  areas[0].h = FIELD_HEIGHT;
  areaCount++;
  splitArea(0);

  for (int y = 0; y < FIELD_HEIGHT; y++)  //フィールドを全て壁で埋める
    for (int x = 0; x < FIELD_WIDTH; x++)
      field[y][x] = CELL_TYPE_WALL;

  for (int i = 0; i < areaCount; i++) { //それぞれのルームを定義
    areas[i].room.x = areas[i].x + 2;
    areas[i].room.y = areas[i].y + 2;
    areas[i].room.w = areas[i].w - 4;
    areas[i].room.h = areas[i].h - 4;

    for (int y = areas[i].room.y; y < areas[i].room.y + areas[i].room.h; y++)  //それぞれのルームをフィールドに書き込み
      for (int x = areas[i].room.x; x < areas[i].room.x + areas[i].room.w; x++)
        field[y][x] = CELL_TYPE_NONE;
    

    for (int x = areas[i].x; x < areas[i].x + areas[i].w; x++)  //共通の通路（横ライン）
      field[areas[i].y + areas[i].h - 1][x] = CELL_TYPE_NONE;

    for (int y = areas[i].y; y < areas[i].y + areas[i].h; y++)  //共通の通路（縦ライン）
      field[y][areas[i].x + areas[i].w - 1] = CELL_TYPE_NONE;

    for (int y2 = areas[i].y; y2 < areas[i].room.y; y2++)     //部屋〜共通通路（上）
      field[y2][areas[i].x + areas[i].w / 2] = CELL_TYPE_NONE;
    
    for (int y2 = areas[i].room.y + areas[i].room.h; y2 < areas[i].y + areas[i].h; y2++)     //部屋〜共通通路（下）
      field[y2][areas[i].x + areas[i].w / 2] = CELL_TYPE_NONE;
    
    for (int x2 = areas[i].x; x2 < areas[i].room.x; x2++)     //部屋〜共通通路（左）
      field[areas[i].y + areas[i].h / 2][x2] = CELL_TYPE_NONE;
    
    for (int x2 = areas[i].room.x + areas[i].room.w; x2 < areas[i].x + areas[i].w; x2++) //部屋〜共通通路（右)
      field[areas[i].y + areas[i].h / 2][x2] = CELL_TYPE_NONE;
  }

  for (int y = 0; y < FIELD_HEIGHT; y++)  //フィールドの右端を壁で埋める
    field[y][FIELD_WIDTH-1] = CELL_TYPE_WALL;

  for (int x = 0; x < FIELD_WIDTH; x++)   //フィールドの下端を壁で埋める
    field[FIELD_HEIGHT-1][x] = CELL_TYPE_WALL;

  while(1) {  //全ての行き止まりを埋めるために終わるまでループ
    int filled = 1;   //埋められたフラグ
    for (int y = 0; y < FIELD_HEIGHT; y++)
      for (int x = 0; x < FIELD_WIDTH; x++) {
        if (field[y][x] == CELL_TYPE_WALL)  //そもそもチェック中の座標が壁なら、次のマスへ
          continue;
        int v[][2] = {  //周りに壁が何個あるかの判定用
          {0, -1},  //左
          {-1, 0},  //上
          {0, 1},   //右
          {1, 0},   //下
        };
        int n = 0;
        for (int i =  0; i < 4; i++) {
          int x2 = x + v[i][0];
          int y2 = y + v[i][1];
          if ((x2 < 0) || (x2 >= FIELD_WIDTH) || (y2 < 0) || (y2 >= FIELD_HEIGHT))
            n++;
          else if (field[y2][x2] == CELL_TYPE_WALL)
            n++;
        }
        if (n >= 3) {
          field[y][x] = CELL_TYPE_WALL;
          filled = 0;
        }
      }
      if (filled)   //埋まり切ったら抜ける
        break;
  }
  
  setRandPosition(&player.x, &player.y);  //プレイヤーのランダムな座標の取得。返り値を二つ受け取れないので、アドレスを渡して書き換える。(ベクトルだったら可能)
  {   //ランダムに階段を設置
    int x, y;
    setRandPosition(&x, &y);
    field[y][x] = (floorNum < 3) ? CELL_TYPE_STAIRS : CELL_TYPE_AMULET;
  }

  setRandPosition(&enemy.x, &enemy.y);  //敵をランダムに配置
  enemy.hp = enemy.maxHp = 9 + floorNum;  //敵のHP
  enemy.attack = 2 + floorNum;  //敵の攻撃力

}

void drawField() {   //フィールドの描画（使わない記念） マップが全て見えてしまうから
  system("clear");
  for (int y = 0; y < FIELD_HEIGHT; y++) {
    for (int x = 0; x < FIELD_WIDTH; x++) {
        if (x == player.x && y == player.y)
          printf("＠");
        else
          printf("%s", cellAA[field[y][x]]);
    }
    printf(": %d\n", y);
  }

}

int getRoom(int _x, int _y) {   //部屋の中にいればそのエリア番号を返す （部屋＋入り口１マスが同じ部屋にいると判定される）
  for (int i = 0; i < areaCount; i++)
    if ((_x >= areas[i].room.x - 1) && (_x < areas[i].room.x + areas[i].room.w + 1)
        && (_y >= areas[i].room.y - 1) && (_y < areas[i].room.y + areas[i].room.h + 1)) {
      return i;
    }
  return -1;  //通路にいる場合、-1
}

int getLarge(int _a, int _b) {  //大きい方の数字を返す
  if (_a >= _b)
    return _a;
  else
    return _b;
}

void display() {  //画面の描画
  memcpy(buffer, field, sizeof field);   //bufferにfieldの中身をコピーする　サイスはfieldに合わせる

  buffer[player.y][player.x] = CELL_TYPE_PLAYER;    //bufferに主人公と敵を書き込む
  buffer[enemy.y][enemy.x] = CELL_TYPE_ENEMY;
  system("clear");
  printf("%dF  HP:%d/%d  distance:%d\n", floorNum, player.hp, player.maxHp, distance);
  for (int y = player.y - SCREEN_RANGE; y < player.y + SCREEN_RANGE; y++) {
    for (int x = player.x - SCREEN_RANGE; x < player.x + SCREEN_RANGE; x++) {
        if ((x < 0) || (x >= FIELD_WIDTH) || (y < 0) || (y >= FIELD_HEIGHT))
          printf("團");
        else 
          printf("%s", cellAA[buffer[y][x]]);
    }
    printf("\n");
  }
}

void endMsg(int _gameClear) { //ゲーム終了時のメッセージ 引数が０なら倒れた　１ならゲームクリア
  if (_gameClear) {
    printf("あなたは宝を手に入れた！\n");
    printf("-CLEAR!-\n");
  } else {
    printf("プレイヤーは倒れた...\n");
    printf("-GAME OVER-\n");
  }
}

void ending(int _gameClear) {   //ゲーム終了時の処理
  for (int y1 = player.y - SCREEN_RANGE; y1 < player.y + SCREEN_RANGE; y1++) { //画面を左上から消す
    for (int x1 = player.x - SCREEN_RANGE; x1 < player.x + SCREEN_RANGE; x1++) {
      usleep(7000);
      system("clear");
      printf("%dF  HP:%d/%d\n", floorNum, player.hp, player.maxHp);
      for (int y2 = player.y - SCREEN_RANGE; y2 < player.y + SCREEN_RANGE; y2++) { //画面を表示
        for (int x2 = player.x - SCREEN_RANGE; x2 < player.x + SCREEN_RANGE; x2++) {
          if ((buffer[y2][x2] == CELL_TYPE_ENEMY) && (player.hp <= 0)) {  //チェックしたマスに鬼がいて、HPが０以下なら鬼を表示
            printf("%s", cellAA[buffer[y2][x2]]);
          } else if ((buffer[y2][x2] == CELL_TYPE_AMULET) && (player.hp > 0)) {   //チェックしたマスに宝があって、HPが０より大きければ宝を表示
            printf("%s", cellAA[buffer[y2][x2]]);
          } else if (((y2 <= y1) && (x2 <= x1))   //左上から消していく
          || (y2 < y1)) {
            printf("  ");
          } else {    //それ以外は待機中
            if ((x2 < 0) || (x2 >= FIELD_WIDTH) || (y2 < 0) || (y2 >= FIELD_HEIGHT))
              printf("團");
            else 
              printf("%s", cellAA[buffer[y2][x2]]);
          }
        }
        printf("\n");
      }
      endMsg(_gameClear);
    }
  }
  mygetch();    //終わった直後のコンソールに文字を入力している可能性があるため
}

void tempMoveTo(int *_x, int *_y, int _toX, int _toY) { //二つの座標を比べて前を後へ近づける  移動先の仮座標（*_x, *_y）がx,yとして使用可に
  if (*_x < _toX)
    *_x += 1;
  if (*_x > _toX)
    *_x -= 1;
  if (*_y < _toY)
    *_y += 1;
  if (*_y > _toY)
    *_y -= 1;
}

int moveCheck(int _x, int _y) {  //移動できるかどうかチェック　できれば１
  if ((buffer[_y][enemy.x] != CELL_TYPE_WALL) && (buffer[enemy.y][_x] != CELL_TYPE_WALL)  //斜め移動の時に壁を挟んでいない
  && (buffer[_y][_x] != CELL_TYPE_WALL)) {
    return 1;
  } else {
    return 0;
  }
}

int getPassageTop(int _room) {  //部屋の上側にある通路のx座標を返す
  for (int x = areas[_room].room.x; x < areas[_room].room.x + areas[_room].room.w; x++) {
    if (field[areas[_room].room.y - 1][x] == CELL_TYPE_NONE) {
      return x;
    }
  }
  return 0;
}

int getPassageBottom(int _room) {  //部屋の下側にある通路のx座標を返す
  for (int x = areas[_room].room.x; x < areas[_room].room.x + areas[_room].room.w; x++) {
    if (field[areas[_room].room.y + areas[_room].room.h][x] == CELL_TYPE_NONE) {
      return x;
    }
  }
  return 0;
}

int getPassageLeft(int _room) {  //部屋の左側にある通路のy座標を返す
  for (int y = areas[_room].room.y; y < areas[_room].room.y + areas[_room].room.h; y++) {
    if (field[y][areas[_room].room.x - 1 ] == CELL_TYPE_NONE) {
      return y;
    }
  }
  return 0;
}

int getPassageRight(int _room) {  //部屋の右側にある通路のy座標を返す
  for (int y = areas[_room].room.y; y < areas[_room].room.y + areas[_room].room.h; y++) {
    if (field[y][areas[_room].room.x + areas[_room].room.w] == CELL_TYPE_NONE) {
      return y;
    }
  }
  return 0;
}

int randomCurveToHorizontal(int _x, int _y, int _moveDirection) {    //結：ランダムで左右どちらかへ進む 引：現在地の座標、進んできた方向
  if (rand() % 2 == 0) { //ランダムで左右どちらかを先にチェック
    if (moveCheck(_x+1, _y)) {  //先にチェックした方が空いていなければ逆へ
      enemy.x = _x + 1;
      _moveDirection = 5;
    } else {
      enemy.x = _x - 1;
      _moveDirection = 4;
    }
  } else {
    if (moveCheck(_x-1, _y)) {
      enemy.x = _x - 1;
      _moveDirection = 4;
    } else {
      enemy.x = _x + 1;
      _moveDirection = 5;
    }
  }
  return _moveDirection;
}

int randomCurveToVertical(int _x, int _y, int _moveDirection) {    //結：ランダムで上下どちらかへ進む 引：現在地の座標、進んできた方向
  if (rand() % 2 == 0) { //ランダムで上下どちらかを先にチェック
    if (moveCheck(_x, _y+1)) {  //先にチェックした方が空いていなければ逆へ
      enemy.y = _y + 1;
      _moveDirection = 7;
    } else {
      enemy.y = _y - 1;
      _moveDirection = 2;
    }
  } else {
    if (moveCheck(_x, _y-1)) {
      enemy.y = _y - 1;
      _moveDirection = 2;
    } else {
      enemy.y = _y + 1;
      _moveDirection = 7;
    }
  }
  return _moveDirection;
}

/*------------------------------------------------------------------------------------------------------------*/
int main() {  
  //フィールドの初期化など--------------------------------------------------------------------------------------------
  int turnCount = 0;
  srand((unsigned int)time(NULL));  //乱数を現在時刻で初期化

  player.hp = player.maxHp = 15;  //主人公のHPを１５とする
  player.attack = 100;  //初期攻撃力

  generateField();  //フィールド（エリア分割済み）を作成

  //メインループ------------------------------------------------------------------------------------------------
  while(1) {
    
    display();  //画面描画１
    
    //プレイヤーの行動処理
    int x = player.x;
    int y = player.y;
    switch (mygetch()) {  //キー操作待ち
      case 'w': y--; break;
      case 's': y++; break;
      case 'a': x--; break;
      case 'd': x++; break;
      case 'q': //左上
        if ((buffer[y][x-1] == CELL_TYPE_NONE) && (buffer[y-1][x] == CELL_TYPE_NONE)) { //斜め移動の禁止
          x--;
          y--;
        }
        break;  //左上
      case 'e': //右上
        if ((buffer[y][x+1] == CELL_TYPE_NONE) && (buffer[y-1][x] == CELL_TYPE_NONE)) { //斜め移動の禁止
          x++;
          y--;
        }
        break;
      case 'c': //右下
        if ((buffer[y][x+1] == CELL_TYPE_NONE) && (buffer[y+1][x] == CELL_TYPE_NONE)) { //斜め移動の禁止
          x++;
          y++;
        } 
        break;
      case 'z': //左下
        if ((buffer[y][x-1] == CELL_TYPE_NONE) && (buffer[y+1][x] == CELL_TYPE_NONE)) { //斜め移動の禁止
          x--;
          y++;
        }
        break;
    }
    
    switch (buffer[y][x]) { //主人公の当たり判定　移動先のマスが何かをチェック
      case CELL_TYPE_NONE:  //何もなければ、移動成功
        player.x = x;
        player.y = y;
        if ((turnCount % 15 == 0) && (player.hp < player.maxHp))  //プレイヤーの自然回復処理 １5ターンに１ずつ回復
          player.hp++;
        break;
      case CELL_TYPE_WALL:  //壁なら移動せず、音を出す
        printf("\a");
        break;
      case CELL_TYPE_STAIRS:  //階段ならマップを新しく生成
        floorNum++;
        generateField();
        break;
      case CELL_TYPE_AMULET:    //宝をゲット(ゲームクリア)
        ending(1);
        return 0;
        break;
      case CELL_TYPE_ENEMY:   //鬼との戦闘
        damage = player.attack + rand() % (player.attack/2); //攻撃力 + 攻撃力/2でランダム
        enemy.hp -= damage;
        printf("シレンの攻撃！ %dポイントのダメージを与えた！\n", damage);
        if (enemy.hp <= 0) {
          printf("鬼Lv%dを倒した！\n", floorNum);
          while(1) {  //敵を画面外にランダム配置
            setRandPosition(&enemy.x, &enemy.y);
            if ((enemy.x < player.x - SCREEN_RANGE) || (enemy.x > player.x + SCREEN_RANGE)
            || (enemy.y < player.y - SCREEN_RANGE) || (enemy.y > player.y + SCREEN_RANGE)) {
              break;
            }
          }
          enemy.hp = enemy.maxHp = 9 + floorNum;  //敵のHP
          enemy.attack = 3 + floorNum;  //敵の攻撃力
        }
        mygetch();
        break;
    }

    display();  //画面描画２（自分のターンが終わった段階）
    
      
    //敵の行動処理
    {
      int moveDirection;  //鬼の移動した方向を保存しておく １〜８
      int x = enemy.x;  //ローカルのx,yを鬼の座標へ置き換え
      int y = enemy.y;
      int toX = 0;  //行きたい方向を保存するため
      int toY = 0;
      int room = getRoom(enemy.x, enemy.y);
      distance = getLarge(abs(player.x - enemy.x), abs(player.y - enemy.y));    //上に表示するため「distance」をいったんグローバル変数化~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      if ((room >= 0 && room == getRoom(player.x, player.y))   //追いかけてくる処理　同じ部屋or１マス開けている状態
        || (1 <= distance && distance <= 2)) {
        
        tempMoveTo(&x, &y, player.x, player.y); //プレイヤーの方向へ追いかける 移動先の座標がx,yで使用可 
        if (moveCheck(x, y)) {   //そもそも移動できるかチェック 斜め壁とか
          switch (buffer[y][x]) {
            case CELL_TYPE_PLAYER:  //移動先がプレイヤーなら戦闘
              damage = enemy.attack + rand() % (enemy.attack/2);
              player.hp -= damage;
              printf("鬼Lv%dの攻撃！ %dポイントのダメージを受けた！\n",floorNum ,damage);
              if (player.hp <= 0) {
                ending(0);
                return 0;
              }
              mygetch();
              break;
            default:
              enemy.x = x;
              enemy.y = y;
              break;
          }
        } else if (field[y][enemy.x] != CELL_TYPE_WALL) { //縦方向へ空いていればそちらへ移動
            enemy.y = y;
        } else if (field[enemy.y][x] != CELL_TYPE_WALL) { //横方向へ空いていればそちらへ移動
            enemy.x = x;
        }
      
      //鬼の自動探索処理
      } else if ((room < 0)
      || (enemy.x < areas[room].room.x) || (enemy.x >= areas[room].room.x + areas[room].room.w)
      || (enemy.y < areas[room].room.y) || (enemy.y >= areas[room].room.y + areas[room].room.h)) {  //鬼が通路or部屋の入り口にいる場合)
        switch (moveDirection) {  //移動方向を保存しておく(通路用)
          case 1 :  //左上

            break;
          case 2:   //上
            if (moveCheck(x, y - 1)) {  //進みたい方向へ壁がない状態
              if ((moveCheck(x+1, y)) || (moveCheck(x-1, y))) {   //どちらかに穴があれば
                if (rand() % 2 == 0) {  //左右に曲がるかもしれない
                  moveDirection = randomCurveToHorizontal(x, y, moveDirection);
                 } else { //曲がらなかったら直進
                  y--;
                  enemy.y = y;
                }
              } else {  //どちらにも壁がない = 直進
                y--;
                enemy.y = y;
              }
            } else {    //進みたい方向に壁がある状態
              moveDirection = randomCurveToHorizontal(x, y, moveDirection);
            }
            break;
          case 3:   //右上

            break;
          case 4 : //左
            if (moveCheck(x - 1, y)) {  //進みたい方向へ壁がない状態
              if ((moveCheck(x, y + 1)) || (moveCheck(x, y - 1))) {   //どちらかに穴があれば
                if (rand() % 2 == 0) {  //左右に曲がるかもしれない
                  moveDirection = randomCurveToVertical(x, y, moveDirection);
                 } else { //曲がらなかったら直進
                  x--;
                  enemy.x = x;
                }
              } else {  //どちらにも壁がない = 直進
                x--;
                enemy.x = x;
              }
            } else {    //進みたい方向に壁がある状態
              moveDirection = randomCurveToVertical(x, y, moveDirection);
            }
            break;
          case 5: //右
            if (moveCheck(x + 1, y)) {  //進みたい方向へ壁がない状態
              if ((moveCheck(x, y + 1)) || (moveCheck(x, y - 1))) {   //どちらかに穴があれば
                if (rand() % 2 == 0) {  //左右に曲がるかもしれない
                  moveDirection = randomCurveToVertical(x, y, moveDirection);
                 } else { //曲がらなかったら直進
                  x++;
                  enemy.x = x;
                }
              } else {  //どちらにも壁がない = 直進
                x++;
                enemy.x = x;
              }
            } else {    //進みたい方向に壁がある状態
              moveDirection = randomCurveToVertical(x, y, moveDirection);
            }
            break;
          case 6:   //左下

            break;
          case 7:   //下
            if (moveCheck(x, y + 1)) {  //進みたい方向へ壁がない状態
              if ((moveCheck(x+1, y)) || (moveCheck(x-1, y))) {   //どちらかに穴があれば
                if (rand() % 2 == 0) {  //左右に曲がるかもしれない
                  moveDirection = randomCurveToHorizontal(x, y, moveDirection);
                 } else { //曲がらなかったら直進
                  y++;
                  enemy.y = y;
                }
              } else {  //どちらにも壁がない = 直進
                y++;
                enemy.y = y;
              }
            } else {    //進みたい方向に壁がある状態
              moveDirection = randomCurveToHorizontal(x, y, moveDirection);
            }
            break;
          case 8:   //右下

            break;
          default:
            break;
        }
      } else {  //鬼が部屋にいる場合
        int n = rand() % 4;
        switch (n) {  //通路を探して向かっていく
          case 0:   //鬼が上側の通路へ向かう
            toX = getPassageTop(room);
            if (toX == 0)
              break;
            tempMoveTo(&x, &y, toX, areas[room].room.y - 1);  //移動先の座標がx,yで使用可
            if (moveCheck(x, y)) {
              enemy.x = x;
              enemy.y = y;
            } else if (field[y][enemy.x] != CELL_TYPE_WALL) { //縦方向へ空いていればそちらへ移動
              enemy.y = y;
              
            } else if (field[enemy.y][x] != CELL_TYPE_WALL) { //横方向へ空いていればそちらへ移動
              enemy.x = x;
            }
            moveDirection = 2;
            break;
          case 1:   //鬼が右側の通路へ向かう
            toY = getPassageRight(room);
            if (toY == 0)
              break;
            tempMoveTo(&x, &y, areas[room].room.x + areas[room].room.w, toY);  //移動先の座標がx,yで使用可
            if (moveCheck(x, y)) {
              enemy.x = x;
              enemy.y = y;
            } else if (field[y][enemy.x] != CELL_TYPE_WALL) { //縦方向へ空いていればそちらへ移動
              enemy.y = y;
            } else if (field[enemy.y][x] != CELL_TYPE_WALL) { //横方向へ空いていればそちらへ移動
              enemy.x = x;
            }
            moveDirection = 5;
            break;
          case 2:   //鬼が下側の通路へ向かう
            toX = getPassageBottom(room);
            if (toX == 0)
              break;
            tempMoveTo(&x, &y, toX, areas[room].room.y + areas[room].room.h);  //移動先の座標がx,yで使用可
            if (moveCheck(x, y)) {
              enemy.x = x;
              enemy.y = y;
            } else if (field[y][enemy.x] != CELL_TYPE_WALL) { //縦方向へ空いていればそちらへ移動
              enemy.y = y;
            } else if (field[enemy.y][x] != CELL_TYPE_WALL) { //横方向へ空いていればそちらへ移動
              enemy.x = x;
            }
            moveDirection = 7;
            break;
          case 3:   //鬼が左側の通路へ向かう
            toY = getPassageLeft(room);
            if (toY == 0)
              break;
            tempMoveTo(&x, &y, areas[room].room.x - 1 , toY);  //移動先の座標がx,yで使用可
            if (moveCheck(x, y)) {
              enemy.x = x;
              enemy.y = y;
            } else if (field[y][enemy.x] != CELL_TYPE_WALL) { //縦方向へ空いていればそちらへ移動
              enemy.y = y;
            } else if (field[enemy.y][x] != CELL_TYPE_WALL) { //横方向へ空いていればそちらへ移動
              enemy.x = x;
            }
            moveDirection = 4;
            break;
        }
      }
    }
    turnCount++;
  }
}




