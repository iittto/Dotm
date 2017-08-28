#include "Recognition.h"

#define CRANGE (0.5)

typedef struct{
  double x,y;
}Point2d;

typedef struct{
  Point2d min,max;
}rect_t;

using namespace std;

void Vector2Diff(Point2d *diff, const Point2d *p, const Point2d *q){
  diff->x = p->x - q->x;
  diff->y = p->y - q->y;
}

double Vector2InnerProduct(const Point2d *p, const Point2d *q){
  return p->x * q->x + p->y * q->y;
}

double Vector2OuterProduct(const Point2d *p, const Point2d *q){
  return p->x * q->y - p->y * q->x;
}

//cを中心にpをangle回転させてrに格納
void Vector2Rotation(Point2d *r, const Point2d *p, const Point2d *c, const double angle){  
  r->x = (p->x - c->x) * cos(angle) - (p->y - c->y) * sin(angle);
  r->y = (p->x - c->x) * sin(angle) + (p->y - c->y) * cos(angle);
  r->x += (c->x);
  r->y += (c->y);
}


void swap_rectangle(rect_t *ch1, rect_t *ch2){

  rect_t ch_t;

  ch_t = *ch1;
  *ch1 = *ch2;
  *ch2 = ch_t;
  
}

void convert_2to4(Point2d p[4] ,rect_t rect){
  
  p[0].x = rect.min.x;
  p[0].y = rect.min.y;
  p[1].x = rect.max.x;
  p[1].y = rect.min.y;
  p[2].x = rect.max.x;
  p[2].y = rect.max.y;
  p[3].x = rect.min.x;
  p[3].y = rect.max.y;
  
}

double question_point_and_circle(Point2d p, Point2d rp,double r){

  double dx=p.x-rp.x;
  double dy=p.y-rp.y;

  if(dx*dx + dy*dy < r*r)    
    return 1;
  else
    return 0;

}

double get_distance(double x, double y, double x1, double y1, double x2, double y2){
  
  double dx,dy,a,b,t,tx,ty;
  double distance;
  
  dx = (x2 - x1); 
  dy = (y2 - y1);
  a = dx*dx + dy*dy;
  b = dx * (x1 - x) + dy * (y1 - y);
  t = -b / a;
  
  if (t < 0) 
    t = 0;
  if (t > 1) 
    t = 1;
  
  tx = x1 + dx * t;
  ty = y1 + dy * t;
  distance = sqrt((x - tx)*(x - tx) + (y - ty)*(y - ty));

  return distance;

}

double get_theta(Point2d pt0,Point2d pt1,Point2d rpt){

  Point2d c, p, q;
  Point2d cp; 
  Point2d cq;
  double s;
  double t;
  double theta;

  c.x = pt0.x;    
  c.y = pt0.y;
  p.x = pt1.x;    
  p.y = pt1.y;
  q.x = rpt.x;    
  q.y = rpt.y;

  Vector2Diff(&cp, &p, &c); 
  Vector2Diff(&cq, &q, &c); 
  s = Vector2OuterProduct(&cp, &cq);
  t = Vector2InnerProduct(&cp, &cq);
  theta = atan2(s, t);

  return theta;

}

double collision_detection_square_and_circle(Point2d pt[4], Point2d rpt, double r){

  int i;
  double x=rpt.x,y=rpt.y;
  double theta1,theta2;

  theta1 = get_theta(pt[0],pt[1],rpt);
  theta2 = get_theta(pt[2],pt[3],rpt);

  if(0<=theta1 && theta1<=PI/2 && 0<=theta2 && theta2<=PI/2)
    return 1;

  for(i=0;i<4;i++){
    if(question_point_and_circle(pt[i],rpt,r)==1)
      return 1;
    if(get_distance(rpt.x,rpt.y,pt[i].x,pt[i].y,pt[(i+1)%4].x,pt[(i+1)%4].y)<r)
      return 1;
  }

  return 0;
  
}

//現在出力対象の文字数を返す
int OutPutCharacterNum(character_t **character, int class_num){
  
  int num=0;
  
  for(int i=0;i<class_num;i++){
    for(int j=0;j<character[i][0].rectnum;j++){
      if(character[i][j].error==0)
	num++;
    }
  }
  
  return num;
  
}

//最適な分離を探す
void SegmentDicision(character_t *character){
  
  //探索用
  int DPtype_num=0;//大分類の数
  int DPnum_max=0,*DPnum_num;//小分類の最大数、小分類それぞれの数
  //計算用
  int result_tmp;
  int *DPresult;//最終結果
  int **DPrectnum;//それぞれ何個あるか（平均のために割る数）
  double min_mean,tmp_mean;
  double **DPmean;
  
  //競合を探す
  for(int i=0;i<character[0].rectnum;i++){
    if(DPtype_num <= character[i].DPtype)
      DPtype_num = character[i].DPtype+1;
  }
  
  DPnum_num = new int [DPtype_num];
  fill(DPnum_num,DPnum_num+DPtype_num,0);
  
  for(int i=0;i<character[0].rectnum;i++){
    //DPnumの種類 *** 「領域なしの0」と「0番が最大」をしっかり区別すること
    if(DPnum_max <= character[i].DPnum)
      DPnum_max = character[i].DPnum+1;//全部の最大数
    if(DPnum_num[character[i].DPtype] <= character[i].DPnum)
      DPnum_num[character[i].DPtype] = character[i].DPnum+1;//クラス内の最大数
  }
  
  //最良を探す
  DPresult = new int [DPtype_num];
  fill(DPresult,DPresult+DPtype_num,0);
  
  DPrectnum = new int    *[DPtype_num];
  DPmean    = new double *[DPtype_num];

  for(int i=0;i<DPtype_num;i++){
    DPrectnum[i] = new int    [DPnum_max];
    DPmean[i]    = new double [DPnum_max]; 
    fill(DPmean[i]   ,DPmean[i]   +DPnum_max ,0);
    fill(DPrectnum[i],DPrectnum[i]+DPnum_max,0);
  }
  
  for(int i=0;i<character[0].rectnum;i++){
    //個数をカウント
    DPrectnum[character[i].DPtype][character[i].DPnum]++; 
    //評価値の平均値を求めるために足す
    if(character[i].error == 0)
      DPmean[character[i].DPtype][character[i].DPnum] += character[i].result[0];
    else
      DPmean[character[i].DPtype][character[i].DPnum] += 1000000; //評価値をあげておく
  }
  
  //確認
  IFcout(2) << "type " << "num " << "numnum " << "mean" << endl;
  for(int i=0;i<DPtype_num;i++){
    for(int j=0;j<DPnum_max;j++){

      if(DPrectnum[i][j] == 0)
	tmp_mean = 1000000;
      else
	tmp_mean = DPmean[i][j] / DPrectnum[i][j];
    
      if(j==0 || min_mean > tmp_mean){
	if(tmp_mean != 0){
	  min_mean = tmp_mean;//最小値
	  result_tmp = j;//最小値を持つ番号
	}
      }
      
      IFcout(2) << i << "    " << j << "   " << DPrectnum[i][j] 
	     << "     " << tmp_mean << endl;
      
    }
    DPresult[i] = result_tmp;
    IFcout(2) << "最小値:" << min_mean << " 採用：" << result_tmp << endl;
  }
  
  IFcout(2) << endl;
  
  //棄却
  for(int i=0;i<character[0].rectnum;i++){
    if(character[i].DPnum != DPresult[character[i].DPtype])
      character[i].error = 1;
  }

  //終了処理
  for(int i=0;i<DPtype_num;i++){
    delete[] DPrectnum[i];
    delete[] DPmean[i];
  }
  
  delete[] DPrectnum;
  delete[] DPmean;
  delete[] DPnum_num;
  delete[] DPresult;
  
}

//擬似ベイズ識別関数評価値で識別する
void BayesThresholdNoiseRemove(character_t *character){
  
  for(int i=0;i<character[0].rectnum;i++){
    if(character[i].result[0] > BAYESTHRES){
      character[i].error = 1;
    }
  }
  
}

//アスペクト比で識別する
void AspectNoiseRemove(character_t *character){

  double x,y;
  
  for(int i=0;i<character[0].rectnum;i++){
    x = character[i].maxx - character[i].minx;
    y = character[i].maxy - character[i].miny;
    if(x>2*y)
      character[i].error = 1;
  }
  
}

//指定された範囲に別の文字が存在するかを判定する 指定方法...文字の大きさのdist倍の範囲
int SearchNearCharacter(character_t **character, int class_num, int c, int n, double dist){ 
  
  int x1,y1,x2,y2;
  double sx,sy;
  
  
  //比較元文字
  x1 = character[c][n].x;
  y1 = character[c][n].y;
  
  //比較文字の大きさ
  sx = character[c][n].maxx - character[c][n].minx;
  sy = character[c][n].maxy - character[c][n].miny;
  
  dist = dist*max(sx,sy);
  
  //全ての文字と比較
  for(int i=0;i<class_num;i++){
    for(int j=0;j<character[i][0].rectnum;j++){

      if(character[i][j].error==0 && (i!=c || j!=n)){
	
	x2 = character[i][j].x;
	y2 = character[i][j].y;
	
	if(hypot(x1-x2,y1-y2)<dist)
	  return 1;//近くに文字あり
	
      }
      
    }
  }
  
  return 0;//近くに文字なし
  
}


//指定された範囲に別の文字が存在するかを判定する
int SearchOverCharacter(character_t **character, int class_num, int c, int n, int &rc, int &rn, double over){ 
  
  rect_t ch1,ch2;
  Point2d c1,c2;
  Point2d F1[4],F2[4];
  Point2d R1[4],R2[4];
  Point2d flat,rotated;
  double minarea,maxarea,overarea;
  double a1,a2;
  
  //比較元文字
  a1   = character[c][n].angle;
  c1.x = character[c][n].x;
  c1.y = character[c][n].y;
  ch1.min.x = character[c][n].minx;
  ch1.min.y = character[c][n].miny;
  ch1.max.x = character[c][n].maxx;
  ch1.max.y = character[c][n].maxy;
  convert_2to4(F1,ch1);
  
  //矩形の回転
  for(int i=0;i<4;i++){
    Vector2Rotation(&R1[i],&F1[i],&c1,a1);
  }
  
  //全ての文字と比較
  for(int i=0;i<class_num;i++){
    for(int j=0;j<character[i][0].rectnum;j++){
      
      if(character[i][j].error==0 && (i!=c || j!=n)){
	
	//比較先文字
	a2   = character[i][j].angle;
	c2.x = character[i][j].x;
	c2.y = character[i][j].y;
	ch2.min.x = character[i][j].minx;
	ch2.min.y = character[i][j].miny;
	ch2.max.x = character[i][j].maxx;
	ch2.max.y = character[i][j].maxy;
	convert_2to4(F2,ch2);
	
	minarea = min((ch1.max.x - ch1.min.x) * (ch1.max.y - ch1.min.y),
		      (ch2.max.x - ch2.min.x) * (ch2.max.y - ch2.min.y));
	maxarea = max((ch1.max.x - ch1.min.x) * (ch1.max.y - ch1.min.y),
		      (ch2.max.x - ch2.min.x) * (ch2.max.y - ch2.min.y));
	
	//重なり面積
	overarea = 0;
	
	
	//重なり面積の測定
	for(int y=F2[0].y; y<=F2[2].y; y++){
	  for(int x=F2[0].x; x<=F2[2].x; x++){
	    flat.y = y;
	    flat.x = x;
	    Vector2Rotation(&rotated,&flat,&c2,a2);
	    overarea += collision_detection_square_and_circle(R1,rotated,CRANGE);
	  }
	}
	
	//矩形周囲長分だけ調整
	if(overarea>0)
	  overarea = pow(sqrt(overarea)-1,2);
	else
	  overarea = 0;
	
	if(minarea*over<overarea){
	 
	  //極端に大きい文字は削除
	  /*
	  if(minarea*5 < maxarea){
	    if((ch1.max.x - ch1.min.x) * (ch1.max.y - ch1.min.y) > 
	       (ch2.max.x - ch2.min.x) * (ch2.max.y - ch2.min.y)){
	      rc = c;
	      rn = n;
	    }
	    else{
	      rc = i;
	      rn = j;
	    }
	    return 1;
	  }
	  */
	  //評価値の悪い方を削除
	  if(character[i][j].result[0] < character[c][n].result[0]){
	    rc = c;
	    rn = n;
	  }
	  else{
	    rc = i;
	    rn = j;
	  }
	  
	  return 1;
	  
	}
	
	
      }
      
    }
  }
  
  return 0;//重なった文字なし
  
}
//回転に対応していない旧版
int SearchOverCharacterOld(character_t **character, int class_num, int c, int n, int &rc, int &rn, double over){ 
  
  int xmin1,ymin1,xmax1,ymax1;
  int xmin2,ymin2,xmax2,ymax2;
  double s1,s2,st,sm,sx[4],sy[4];//面積計算用バッファ
  
  //比較元文字
  xmin1 = character[c][n].minx;
  ymin1 = character[c][n].miny;
  xmax1 = character[c][n].maxx;
  ymax1 = character[c][n].maxy;
  
  
  //全ての文字と比較
  for(int i=0;i<class_num;i++){
    for(int j=0;j<character[i][0].rectnum;j++){
      
      if(character[i][j].error==0 && (i!=c || j!=n)){
	
	xmin2 = character[i][j].minx;
	ymin2 = character[i][j].miny;
	xmax2 = character[i][j].maxx;
	ymax2 = character[i][j].maxy;
	

	//重なり率算出
	sx[0] = xmax1;
	sx[1] = xmin1;
	sx[2] = xmax2;
	sx[3] = xmin2;
	sy[0] = ymax1;
	sy[1] = ymin1;
	sy[2] = ymax2;
	sy[3] = ymin2;
	
	if( xmin1 <= xmax2 && xmin2 <= xmax1 && ymin1 <= ymax2 && ymin2 <= ymax1 ){
	  
	  s1 = (xmax1 - xmin1) * (ymax1 - ymin1);
	  s2 = (xmax2 - xmin2) * (ymax2 - ymin2);
	  sm = min(s1,s2);//小さい方の面積
	  
	  sort(sx,sx+4);
	  sort(sy,sy+4);
	  st =  (sx[2] - sx[1]) * (sy[2] - sy[1]);//重なり面積
	  
	  if(sm*over<st){
	    if(character[i][j].result[0] < character[c][n].result[0]){
	      rc = c;
	      rn = n;
	    }
	    else{
	      rc = i;
	      rn = j;
	    }
	    return 1;
	  }
	  
	}
	
      }
      
    }
  }
  
  return 0;//重なった文字なし
  
}


//付近に文字が無いなら除去
void PositionNoiseRemove(character_t **character, int class_num){

  for(int i=0;i<class_num;i++){
    for(int j=0;j<character[i][0].rectnum;j++){
      if(character[i][j].error == 0 && SearchNearCharacter(character,class_num,i,j,3.0) == 0)
	character[i][j].error = 1;
    }
  }
  
}

//重なった領域で不要なものを除去する
void OverlapNoiseRemove(character_t **character, int class_num){
  
  int m,n;
  int saiki = 0;
  
  for(int i=0;i<class_num;i++){
    for(int j=0;j<character[i][0].rectnum;j++){
      if(character[i][j].error == 0 && SearchOverCharacterOld(character,class_num,i,j,m,n,0.2) != 0){
	character[m][n].error = 1;
	saiki = 1;
      }
    }
  }
  
  if(saiki > 0)
    OverlapNoiseRemove(character,class_num);
  
}


//平均評価値の低い領域を削除
void RegionVoteRemove(character_t **character, int class_num){
  
  int t,f;
  
  for(int i=0;i<class_num;i++){
    
    t=f=0;
    
    for(int j=0;j<character[i][0].rectnum;j++){  
      if(character[i][j].error == 0){
	if(character[i][j].result[0]<BAYESTHRES/2)
	  t++;
	else
	  f++;
      }
    }
    
    if(f>t){
      for(int j=0;j<character[i][0].rectnum;j++){
	character[i][j].error = 1;
      }
    }
    
  }
  
}

//平均評価値と領域の位置関係による棄却
void RegionPositionRemove(character_t **character, int class_num){
    
  int num=0,max=0,remove_flag=0;
  double mean=0,best=10000,yaxis=0,xaxis=0;
  double a,b,c,x,y,h,d;
  
  //最も平均評価値の良い領域を探索，同時に位置と高さも求める
  for(int i=0;i<class_num;i++){
    num  = 0;
    mean = 0;
    xaxis = 0;
    yaxis = 0;
    //領域の中心と高さを求める
    for(int j=0;j<character[i][0].rectnum;j++){ 
      if(character[i][j].error == 0){
	num ++;
	mean  += character[i][j].result[0];
	xaxis += character[i][j].x;
	yaxis += character[i][j].y;
      }
    }
    
    if(num>0){
      mean  /= num;
      xaxis /= num;
      yaxis /= num;
    }
    
    if(best>mean && num>2){
      best = mean;
      max  = i;
      x = xaxis;
      y = yaxis;
      remove_flag = 1;
    }
    
  }
  
  //評価することが不可能ならば終了
  if(remove_flag==0)
    return;
  
  //位置と大きさによって文字を除去
  a = tan(character[max][0].angle);
  b = -1;
  c = 0;
  h = character[max][0].rheight;
  yaxis = y;
  xaxis = x;
  
  for(int i=0;i<class_num;i++){
    for(int j=0;j<character[i][0].rectnum;j++){ 
      
      x = character[i][j].x - xaxis;
      y = character[i][j].y - yaxis;
      d = fabs((double)(a*x+b*y+c)) / sqrt(a*a+b*b);
      
      //位置
      if(3*h < d)
	character[i][j].error = 1;
      
      //大きさ
      if(2*h < character[i][j].rheight)
	character[i][j].error = 1;
      
    }
  }
  
}


void RecognitionPostprocess(character_t **character, int class_num){
  
  IFcout(2) << "最良分離選択" << endl;
  
#ifdef _OPENMP
#pragma omp parallel for
#endif
  for(int i=0;i<class_num;i++){
    IFcout(2) << "class " << i << ":" << endl;
    
    //アスペクト比による選択
    AspectNoiseRemove(character[i]);
    //最良分離矩形選択
    SegmentDicision(character[i]);  
    
    if(class_num==1)
      return;
      
//   cout << "棄却前" << OutPutCharacterNum(character,class_num) << "文字" << endl;
   
    //しきい値による棄却
    BayesThresholdNoiseRemove(character[i]);
  }

  //位置関係による棄却
  IFcout(2) << "MQDF値による棄却後" << OutPutCharacterNum(character,class_num) << "文字" << endl;
  OverlapNoiseRemove(character,class_num);
  IFcout(2) << "重なりによる棄却後 " << OutPutCharacterNum(character,class_num) << "文字" << endl;
  PositionNoiseRemove(character,class_num);
  IFcout(2) << "周辺の位置で棄却後" <<  OutPutCharacterNum(character,class_num) << "文字" << endl;
  RegionVoteRemove(character,class_num);
  IFcout(2) << "領域単位での棄却後" <<  OutPutCharacterNum(character,class_num) << "文字" << endl;
  RegionPositionRemove(character,class_num);
  IFcout(2) << "領域位置での棄却後" << OutPutCharacterNum(character,class_num) << "文字" << endl;
  
}
