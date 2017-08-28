/*********************************************************************

   2014-2016
   Human Interface Laboratory Koshi Suzuki (Higabaka United Cauchy)
   Presents
   
   "撮影条件の違いに頑健なドットマトリクス文字の抽出と認識"
   "Extraction and Recognition of Dot-matrix Characters"


**********************************************************************/

#include "MainHeader.h"

int main (int argc, char **argv){
  
  char name[256];
  clock_t check_point; //時間計測開始地点（出力が地点になる）
  
  IplImage *src_img[SRCIMG_NUM]; //オリジナル画像をそのまま保存しておく
  IplImage *dst_img[DSTIMG_NUM]; //オリジナル画像をアレンジする
  

  //例外処理として終了
  if(argc < 3){
    cout << "usage       :  ./main [Input] [Output ... ALL or NULL or \"Filename\"] [PrintFlag ... 0 or 1 or 2 or 3]" << endl;
    cout << "[Input]     :  必須です. 拡張子を含め入力ファイルのパスを記述してください." << endl;
    cout << "[Output]    :  必須です. 出力ファイルのパスを記述してください．\"Filename\"の時は拡張子が必要です." << endl;
    cout << "[PrintFlag] :  省略可能です. 途中経過出力の細かさを指定します．デフォルトで0." << endl;
    exit(1);
  }

  //引数によってメッセージの設定を変更
  if(argc > 3)
    OUT = atoi(argv[3]);
  else
    OUT = 0;
  
  //引数エラーチェック
  if(argc > 3 && (strlen(argv[3])!=1 || !isdigit(argv[3][0]))){
    cout << "[PrintFlag] : [引数3]は0-9の数字で指定してください." << endl;
    exit(1);
  }
  
  //名前部分の切り出し
  setName(name,argv[1]);
  
  //画像読み込み
  if(LoadImage(src_img, dst_img, argv[1]) != 0){
    cout << "[Input] : 画像の読み込みに失敗しました." << endl;
    exit(1);
  }
  


  /*********************
      以下メインプログラム
  **********************/ 
  







  IFcout(1) << endl << "----------　　　候　補　点　抽　出　　　----------" << endl << endl;

  //検出したコーナーを格納するための変数
  vector<KeyPoint> keypoints;  //常に変化する計算用
  vector<KeyPoint> savepoints; //最初に取得したポイントの保存用
  
#if EXTRACTION_LEVEL > 0 //文字の位置が完全に与えられたときの実験

  KeypointGrandTruth(keypoints, name);

#else
  
  //検出したPatchについて勾配の強度と方向を得るための変数
  double **Grad = new double *[dst_img[1]->height];
  double **Dir  = new double *[dst_img[1]->height]; 

  for(int i=0; i<dst_img[1]->height; i++){
    Grad[i] = new double [dst_img[1]->width];
    Dir[i]  = new double [dst_img[1]->width];
  }
   
  check_point = clock();
  
  //FASTコーナー点を検出、平滑化、keypointsに点を格納
  CornerDetection(dst_img[2], keypoints);
  
  IFtime(3)<<1000.0*static_cast<double>(clock() - check_point)/CLOCKS_PER_SEC<<" ";

  //dst_img[2]は適切な回数平滑化が行われているのでコピーする
  dst_img[0] = cvCloneImage(dst_img[2]);
  dst_img[1] = cvCloneImage(dst_img[2]);
  
  //検出した全ポイントを保存しておく（確認用）
  savepoints.resize( keypoints.size() );
  copy(keypoints.begin(),keypoints.end(),savepoints.begin());
  
  //ソーベルフィルタで勾配強度と方向を取得、GradとDirに格納
  SobelOperator(dst_img[1], Grad, Dir);
  
  //Cannyによるエッジ画像の作成、正解確認用
  cvCanny(src_img[0], dst_img[0], 10.0, 50.0);
  
  //CannyフィルタによるPatchの削除
  CannyPatchEliminate(dst_img[0],keypoints);
  
  //勾配特徴によるPatchの削除
  GradientPatchEliminate(dst_img[0],keypoints,Dir,Grad);
  
  //色強度によるPatchの削除
  ColorPatchEliminate(dst_img[3],keypoints);
  
  //連結数によるPatchの削除
  ConnectPatchEliminate(dst_img[0],keypoints,0);
  

  // ポインタ配列の解放
  for(int i=0; i<dst_img[1]->height; i++ ) {
    delete[] Grad[i];
    delete[] Dir[i];
  }
  
  delete[] Grad;
  delete[] Dir;









  IFcout(1)  << endl << "----------　　候　補　領　域　抽　出　　----------" << endl << endl;
  
  int label = 0;
  vector<KeyPoint> classpoints0(keypoints.size()); //クラス分類用1
  vector<KeyPoint> classpoints1(keypoints.size()); //クラス分類用2
  vector<KeyPoint> classpoints2(keypoints.size()); //クラス分類用3
  
  copy(keypoints.begin(), keypoints.end(), classpoints0.begin());
  copy(keypoints.begin(), keypoints.end(), classpoints1.begin());
  copy(keypoints.begin(), keypoints.end(), classpoints2.begin());
  
  check_point = clock();
  
  //ポイントを「距離、輝度ヒストグラム」の特徴によってクラス分けする
  PointClustering(dst_img[3], classpoints0, 0, label);
  PointClustering(dst_img[3], classpoints1, 1, label);
  PointClustering(dst_img[3], classpoints2, 2, label);

  keypoints.clear();
  keypoints.insert(keypoints.end(), classpoints0.begin(), classpoints0.end());
  keypoints.insert(keypoints.end(), classpoints1.begin(), classpoints1.end());
  keypoints.insert(keypoints.end(), classpoints2.begin(), classpoints2.end());
  
  IFtime(3)<<1000.0*static_cast<double>(clock() - check_point)/CLOCKS_PER_SEC<<" ";
  
#endif
  







  
  IFcout(1)  << endl << "----------　　　文　字　列　分　離　　　----------" << endl << endl;
  
  int class_num = getClassNum(keypoints);  
  class_info_t *cinfo = new class_info_t [class_num];//ドット文字候補情報
  
  //候補点から候補領域へ変換、class_numを候補点クラス数からドット文字候補領域の数に変更
  ConvertKeypointsToCinfo(cinfo, class_num, keypoints);
  
  region_t *region = new region_t [class_num];

  //結果画像切り出し、リロード
  LoadCandidateRegion(region, cinfo, class_num, dst_img[4], name);

#if EXTRACTION_LEVEL > 1  //文字の位置が完全に与えられたときの実験

  RegionGrandTruth(region, name);

#else
  
  check_point = clock();
  
  //分離の前処理（行検出と膨張）
  SegmentationPreprocess(region, class_num);
  
  //分離処理（連結成分分析と位置修正と保存）
  SegmentationMain(region, class_num, name);
  
  //クラスが無い場合、今後の処理をしない
  if(class_num==0){
    IFcout(1) << "No characters." << endl;
    exit(1);
  }
  
  IFtime(3)<<1000.0*static_cast<double>(clock() - check_point)/CLOCKS_PER_SEC<<" ";
  
#endif
  








  IFcout(1)  << endl << "----------　　　　文　字　認　識　　　　----------" << endl << endl;
  
  character_t **character = new character_t *[class_num];
 
  //構造変換及び文字の初期化
  ConvertRegionToCharacter(dst_img[0], region, character, class_num);

  //特徴抽出
  check_point = clock();
  FeatureExtraction(character, class_num, name);
  IFtime(3)<<1000.0*static_cast<double>(clock() - check_point)/CLOCKS_PER_SEC<<" ";
  
  //認識評価値算出
  check_point = clock();
  RecognitionMain(character, class_num, name);
  IFtime(3)<<1000.0*static_cast<double>(clock() - check_point)/CLOCKS_PER_SEC<<" ";
   
//   OutputResult(character, class_num);
//   cout << "--------------------------------------" << endl;
   
  //認識結果を用いた文字判別処理
  check_point = clock();
  RecognitionPostprocess(character, class_num);
  IFtime(3)<<1000.0*static_cast<double>(clock() - check_point)/CLOCKS_PER_SEC<<" ";
  

/*
  if(OUT!=3&&OUT!=0)
    OutputResult(character, class_num, name);
  else if(OUT!=0)
    OutputResult(character, class_num);
*/

//  cout << "--------------------------------------" << endl;






  
  /*********************
      以上メインプログラム
  **********************/ 
  //時間計測時に必要（評価用フォーマット合わせ）
  IFtime(3) << endl;

  //評価用の位置をセット
  ReplaceCharacterArea(character, class_num);
  
  //確認用に描画する
  GraphImage(dst_img[4],keypoints,savepoints,cinfo,region,character,class_num);
  
  //実験結果出力
  if(OUT!=3&&OUT!=0)
    OutputResult(character, class_num, name);
  else if(OUT!=0)
    OutputResult(character, class_num);
  
  //画像の表示
  if(OUT!=3&&OUT!=0){
    SaveImage(src_img, dst_img, argv[2]);
    DisplayImage1(region, class_num);
    DisplayImage2(src_img, dst_img);
  }
  
  //画像の開放
  ReleaseImage(src_img, dst_img);
  

  // ポインタ配列の解放
  for(int i=0;i<class_num;i++){
    delete[] character[i];
  }
  delete[] cinfo;
  delete[] region;
  delete[] character;

  IFcout(1) << endl << "正　常　に　終　了" << endl << endl;

  
  return 0;
  
}
