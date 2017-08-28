#include "SubHeader.h"

//候補領域抽出後の最初の処理、キーポイントを渡すとクラス個数を返す
int getClassNum(vector<KeyPoint> keypoints){
  
  int num = 0;
  vector<KeyPoint>::iterator it_kp;
  
  for(it_kp = keypoints.begin(); it_kp != keypoints.end(); it_kp++){
    if(num < it_kp->class_id + 1)
      num = it_kp->class_id + 1;
  }
  
  return num;
}

//keypoints から class_info_tに変換する
void setClassInformation(class_info_t *cinfo,  int class_num, vector<KeyPoint> keypoints){
   
  int n;
  vector<KeyPoint>::iterator it_kp;
  
  //各クラスのFAST点の個数を取得
  for(it_kp = keypoints.begin(); it_kp != keypoints.end(); it_kp++){
    cinfo[it_kp->class_id].pnum ++;
  }
  
  //座標格納のために個数分の配列を確保
  for(int i=0;i<class_num;i++){
    cinfo[i].pt = new Point2d [cinfo[i].pnum];
  }
  
  for(int i=0;i<class_num;i++){
    n = 0;
    for(it_kp = keypoints.begin(); it_kp != keypoints.end(); it_kp++){
      if(i == it_kp->class_id){
	cinfo[i].pt[n] = it_kp->pt;
	n++;
      }
    }
  }
  

}

//cinfoでドット文字候補領域で無いものを削除する
void sortClassInformation(class_info_t *cinfo,  int class_num, int n){
  
  int num=0;
  
  for(int i=0;i<class_num;i++){ 

    if(cinfo[i].DOT > 0){
      
      cinfo[num] = cinfo[i];
      num++;
      
      if(num==n){
	for(int j=num;j<class_num;j++){
	  memset(cinfo+j,0,sizeof(class_info_t));//ノイズ領域は消しておく
	  delete[] cinfo[j].pt;
	}
	return;
      }
      
    }
  }
  
}


//クラス情報を元に回転角度を推定する
void ConvertKeypointsToCinfo(class_info_t *cinfo,int &class_num, vector<KeyPoint> keypoints){
  
  int n=0;
  Point pt;
  Point2f vtx[4];
  RotatedRect box; 
  vector<Point> points;
  
  //候補領域が消えてしまったら終了
  if(class_num == 0){
    perror("No candidate points.\n");
    exit(1);
  }
  
  memset(cinfo,0,sizeof(class_info_t)*class_num);//初期化
  
  setClassInformation(cinfo,class_num,keypoints);

  for(int i=0;i<class_num;i++){
    
    points.clear();
    
    for(int j=0;j<cinfo[i].pnum;j++){
      pt = cinfo[i].pt[j];
      points.push_back(pt);
    }
    
    box = minAreaRect(points); 
    box.points(vtx);
    cinfo[i].angle = box.angle * PI / 180;

 
/*    //四隅の座標と中心を登録
    	cinfo[i].R[0].x = vtx[0].x - (vtx[1].x - vtx[0].x)/7;
    	cinfo[i].R[0].y = vtx[0].y - (vtx[2].y - vtx[0].y)/7;
    	cinfo[i].R[1].x = vtx[1].x + (vtx[1].x - vtx[0].x)/7;
    	cinfo[i].R[1].y = vtx[1].y - (vtx[2].y - vtx[1].y)/7;
    	cinfo[i].R[2].x = vtx[2].x + (vtx[2].x - vtx[3].x)/7;
    	cinfo[i].R[2].y = vtx[2].y + (vtx[2].y - vtx[1].y)/7;
    	cinfo[i].R[3].x = vtx[3].x - (vtx[2].x - vtx[3].x)/7;
    	cinfo[i].R[3].y = vtx[3].y + (vtx[3].y - vtx[0].y)/7;
//*/

    for(int j=0;j<4;j++){
      cinfo[i].R[j] = vtx[j];
      cinfo[i].center.x += cinfo[i].R[j].x/4;
      cinfo[i].center.y += cinfo[i].R[j].y/4;
    }
    
    //面積が十分に大きければ候補領域とする
    if(box.size.width  > DOT_AREA_THRESHOLD && 
       box.size.height > DOT_AREA_THRESHOLD ){
      
      n++;
      cinfo[i].DOT = 1;
      
      //ただし同じ領域を取得した場合は順番が遅い方を削除
      for(int j=0;j<i;j++){
	if(cinfo[i].R[0] == cinfo[j].R[0] && 
	   cinfo[i].R[1] == cinfo[j].R[1] && 
	   cinfo[i].R[2] == cinfo[j].R[2] && 
	   cinfo[i].R[3] == cinfo[j].R[3] ){
	  cinfo[i].DOT = -j-1;
	  n--;
	  break;
	}
      }
    }
    
    //結果出力
    if(cinfo[i].DOT<0){
      IFcout(2) << i << ":" << "棄却(" << -cinfo[i].DOT-1 << ")" << endl;
    }
    else if(cinfo[i].DOT>0){
      IFcout(2) << i << ":採択☆" << " " << cinfo[i].angle * 180  / PI << endl;
    }
    else{
      IFcout(2) << i << ":棄却" << endl;
    }
  }
  
  sortClassInformation(cinfo,class_num,n);
  
  IFcout(2) << endl << "最終的に" << n << "個の領域が候補として残っています" << endl << endl;
  
  //クラス数を更新
  class_num = n;
  
  //候補領域が消えてしまったら終了
  if(class_num == 0){
    perror("No candidate regions.\n");
    exit(1);
  }
  
}
