
/********************************************************************************
	       認識プログラム(pthreadによる並列処理化)
*********************************************************************************/
#include "Recognition.h"

/* 変数変換べき乗数のデフォルト */
#define         POW     0.4//0.4
/* 最大固有軸数 */
#define         KK      40 //50
/* 最大次元数 */
#define         IU      288 //1000
/* 大分類の候補数 */
#define         CDAIBUN 100
/* その他 */
#define NS      500        /*サンプル数に関するパラメータ（500に固定）*/
/*デフォルト値*/
#define ALPHA   0.1       /*疑似ベイズパラメータのデフォルト値 0.5*/
/* 同時に行うスレッド数 */
#define THREAD  10000

extern "C" {
void submqd5b_(float*,float *,float*,float*,int*,int*,int*,int*,float*,float*);
void submqd5b();
void subedf_();
}

/* 並列処理を行う関数 */
void *mqdf_recognition(void *set);

/* 実験的に結果を出す関数 */
char EntryCode(character_t &);

/* 識別処理用変数 */
int  iu2 = IU, kk = KK; //iu2 - 次元数，kk - 使用する固有ベクトル数
float  alpha = ALPHA;   //
float  power = POW;     //
int catsize;  //クラス数
static float  *evec[CATMAX];

/* 識別関数に必要な値の構造体 */
typedef struct{ 
  float ss;
  unsigned short dic_code[CATMAX];
  float *mv[CATMAX], *eval[CATMAX], mv2[CATMAX];
  int ns[CATMAX], n0[CATMAX]; 
} data_set;


/* 識別関数の評価値とクラスラベルの構造体 */
typedef struct{
  int icatp; 
  float sdvec;
} result;

/* 並列処理関数でも参照できるように大域変数で宣言 */
data_set set;
/* スレッド間で変数の保護を行う(同期処理) */
pthread_mutex_t mutex, mutex2, mutex3, mutex4, mutex5; 
/* 認識結果のカウント用 */
int t_errsum, t_count, t_errsum2, t_errsum3,t_errsum4;

void RecognitionMain(character_t **character, int class_num, char *name){
  
  /* ファイル読み込み */
  FILE *fp1;
  char c;
  int cat;
  int n;

  float work[2*IU];
  int kd;

  int  i, j, k, l;
  int ill;
  
  /* 並列処理用変数 */
  int thread_num = THREAD;  //クラス数
  int res = 0;              //並列処理のエラー判定
  int num = 0;
  pthread_t pth[THREAD];

  /* ディレクトリ操作用変数 */
  DIR *dp;
  char dir[256];
  struct dirent *entry;
  struct stat statbuf;
  char *fname, *fname2;
  char orgname[256];
  long fname3;   

  char argv0[16] = "dic288.k100";
  char argv1[16] = "TMP3";

  /* 辞書ファイルのオープン */
  fp1 = fopen( argv0, "r");
  if(fp1 == NULL){ printf("Cannot open dictionary-file : %s\n", argv0); exit(-1); }

  /* 辞書の読みだし */
  fread(&kd, 4, 1, fp1); // 固有軸数
  
  if(kk >= kd){
    printf("-k オプションの値を%d以下にして下さい。\n",kd);
    exit(-1);
  }
  
  cat = 0;
  
  while( fread(&(set.dic_code[cat]), sizeof(unsigned short), 1, fp1) != 0){
    //平均ベクトル
    set.mv[cat] = (float *)malloc( iu2 * sizeof(float) );
    fread(set.mv[cat], sizeof(float), iu2, fp1);
    
    //固有値
    set.eval[cat] = (float *)malloc( iu2 * sizeof(float) );
    fread(set.eval[cat], sizeof(float), iu2, fp1);
    
    //固有ベクトル
    evec[cat] = (float *)malloc( kd*iu2*sizeof(float) );
    fread(evec[cat], sizeof(float), kd*iu2, fp1); 
    
    cat++;
  }
  
  fclose(fp1);
  
  catsize = cat;  //クラス数（カテゴリ数）
  
  /* ユークリッド距離高速計算のための MM の計算 */
  for(cat = 0; cat < catsize; cat++){
    set.mv2[cat] = 0.0;
    for(k = 0; k < iu2; k++){
      set.mv2[cat] += set.mv[cat][k] * set.mv[cat][k];
    }
    set.mv2[cat] *= 0.5;
  }

  /* 疑似ベイズパラメータの計算 */
  for(cat = 0; cat < catsize; cat++){
    set.ns[cat] = NS;
    set.n0[cat] = set.ns[cat] * alpha / (1.0 - alpha);
  }

  set.ss = 0.0;
  
  for(cat = 0; cat < catsize; cat++){
    for(j = 0; j < iu2; j++){
      set.ss += set.eval[cat][j];
    }
  }
  
  set.ss /= catsize * iu2;
  
  /* 特徴ファイルの読み出しと認識 */
  
  t_errsum = 0;
  t_errsum2 = 0;
  t_errsum3 = 0;
  t_errsum4 = 0;
  t_count = 0;

  /* 同期処理用変数の初期化 */
  pthread_mutex_init(&mutex, NULL);
  pthread_mutex_init(&mutex2, NULL);
  pthread_mutex_init(&mutex3, NULL);
  pthread_mutex_init(&mutex4, NULL);
  pthread_mutex_init(&mutex5, NULL);

  num = 0;
  
  for(int i=0;i<class_num;i++){
    for(int j=0;j<character[i][0].rectnum;j++){
      
      if(character[i][j].error == 0){
	/* スレッドの作成 */
	res = pthread_create(&(pth[num]), NULL, mqdf_recognition, &character[i][j]);
	if(res != 0){
	  perror("Thread Creation Failed\n");
	  exit(EXIT_FAILURE);
	}
	
	num++;
	
	/* スレッドの同期処理 */
	/* 並列スレッド数が定義した個数を超えたら一度スレッド作成を停止する */
	/* 同時に並列処理可能なスレッド数は約320個 */
	if(num == thread_num){
	  for(j = 0; j < num; j++){
	    res = pthread_join(pth[j], NULL);
	    if(res != 0){ 
	      perror("Thread Creation Failed\n");
	      exit(EXIT_FAILURE);
	    }
	  }
	  num = 0;
	}	    
	//}
	//////////////////////////////////////////////////////
      }

    }
  } 
  
  /* 残りの端数のスレッド同期処理 */
  for(int i = 0; i < num; i++){
    res = pthread_join(pth[i], NULL);
    if(res != 0){
    }  
  }
  
  /* 結果の格納 */
  for(int i=0;i<class_num;i++){
    for(int j=0;j<character[i][0].rectnum;j++){
      EntryCode(character[i][j]);
    }
  }
  
}

/* クイックソート関数 */
int sdcomp2(const void *sd1, const void *sd2 ){
  result c1 = *(result *)sd1;
  result c2 = *(result *)sd2;
  if ( c1.sdvec < c2.sdvec )
    return -1;
  else if ( c1.sdvec > c2.sdvec )
    return 1;
  else
    return 0;
}

/**************************************************************************/
/*               並列処理関数(疑似ベイズ識別関数による認識)                  */
/**************************************************************************/
void *mqdf_recognition(void *cls){
  character_t *arg = (character_t *)cls;
  data_set *arg2 = &set;
  result re[CATMAX];
  float data2[IU];
  int i, j, k;
  
  //変数変換（べき乗）
  for( j = 0; j < iu2; j++ ){
    data2[j] = pow(arg->hist[j], power );
  }
  
  for(j = 0; j < catsize; j++){ 
    re[j].icatp = j;
  }
  
  // 疑似ベイズ識別関数による詳細分類
  if( kk > 0 ){
    for(j = 0; j < catsize; j++){
      submqd5b_(data2, arg2->mv[re[j].icatp], arg2->eval[re[j].icatp], evec[re[j].icatp], &iu2, &kk, &(arg2->ns[re[j].icatp]), &(arg2->n0[re[j].icatp]), &(arg2->ss), &(re[j].sdvec));
    }
    qsort(re, catsize, sizeof(result), sdcomp2);
  }
    
  //複数のスレッドで同時に書き込みが行われないようロック状態にする
  //pthread_mutex_lock(&mutex);
  
  //character型に登録
  arg->rank1  = arg2->dic_code[re[0].icatp]; 
  arg->catnum = catsize;
  for(j = 0; j < catsize; j++){
    arg->result[j] = re[j].sdvec;
  }
  
  //特徴抽出結果出力
  /*
    printf("\n\n%d\n",iu2);
    for( j = 0; j < iu2; j++ ){
    printf("%f\n",data2[j]);
    }
  */

  //printf("RANK1:%hx RANK2:%hx RANK3:%hx No.%d-%d\n", arg2->dic_code[re[0].icatp], arg2->dic_code[re[1].icatp], arg2->dic_code[re[2].icatp], arg->cls, arg->num);
  //t_count++;  //テストサンプル数合計
  //アンロック
  //pthread_mutex_unlock(&mutex);
  
  pthread_exit(NULL);

}


char EntryCode(character_t &character){
  
  int tmp;
  int code = character.rank1;
  
  if(code == 0x815E){
    character.ch = '/';
  }
  else if(code == 0x8146){
    character.ch = ':';
  }
  else if(code <= 0x8258){
    tmp = 0x8258 - code;
    character.ch = (char)((int)'9' - tmp);
  }
  else if(code >= 0x8260 && code <= 0x8279){
    tmp = 0x8279 - code;
    character.ch = (char)((int)'Z' - tmp);
  }
  else
    character.error = 1;
  
}
