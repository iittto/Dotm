#include "SubHeader.h"

extern int ArrayThreshold(double *, int);

//指定範囲を二値化
void DesignatedBinarization(IplImage *img, int start, int end, int threshold){

  for(int i=0;i<img->height;i++){
    for(int j=start;j<end;j++){
      if((int)((unsigned char)img->imageData[img->widthStep*i+j])>threshold)
	img->imageData[img->widthStep*i+j] = 255;
      else
	img->imageData[img->widthStep*i+j] = 0;
    }
  }
  
}


void LocalBinarization(IplImage *img){
  
  int p,start,end;
  int divnum;
  double hist[256];
  
  //分割数を自動決定
  divnum = (int)((double)img->width/img->height+0.5);
  
  if(divnum==0)
    divnum = 1;
  
  for(int num=0; num<divnum; num++){
    
    fill(hist,hist+256,0);
    
    //画素の正規化ヒストグラムを作成
    for(int i=0; i<img->height; i++){
      
      start = num*img->width/divnum;
      
      if(num<divnum-1)
	end = (num+1)*img->width/divnum;
      else
	end = img->width;
      
      for(int j=start;j<end;j++){
	p = (int)((unsigned char)img->imageData[img->widthStep*i+j]);
	hist[p] += 1.0 / (img->height*(end-start));
      }
    }
    
    DesignatedBinarization(img,start,end,ArrayThreshold(hist,256));
    
  }
    
}


//90度回転して保存
void Rotate90save(IplImage *img, int sx, int sy, int width, int height, char str[]){
  
  //受け取りは普通の順番、ここでWidthとHeightを逆転させた配列を宣言
  IplImage *dst = cvCreateImage( cvSize(height,width),IPL_DEPTH_8U, 3); 
  CvMat* rotationMat;
  int x,y;
  
  for(int i=sy;i<sy+height;i++){
    for(int j=sx;j<sx+width;j++){
      y = i-sy;
      x = width-(j-sx+1);
      dst->imageData[dst->widthStep*x + y*3    ] = img->imageData[img->widthStep*i + j*3];
      dst->imageData[dst->widthStep*x + y*3 + 1] = img->imageData[img->widthStep*i + j*3 + 1];
      dst->imageData[dst->widthStep*x + y*3 + 2] = img->imageData[img->widthStep*i + j*3 + 2];
    }
  }
  
  cvSaveImage(str, dst);
  cvReleaseImage (&dst);
  
} 

//候補領域情報をセットする
void setCandidateRegion(region_t &region, class_info_t cinfo, char *name){
  
  IplImage *tmp;
//  IplImage *dst_img;
//  dst_img = cvCloneImage(region.dot_img);
  
  //候補領域画像のロード
  tmp = cvLoadImage(name, CV_LOAD_IMAGE_GRAYSCALE);
  region.dot_img = cvCreateImage(cvSize((int)(tmp->width*IMAGE_SIZE), (int)(tmp->height*IMAGE_SIZE)), IPL_DEPTH_8U, 1);
  cvResize(tmp, region.dot_img, CV_INTER_LINEAR);


//通常の二値化
//  LocalBinarization(region.dot_img);
//適応的しきい値で二値化
	adaptiveThreshold(cvarrToMat(region.dot_img), cvarrToMat(region.dot_img), 255, CV_ADAPTIVE_THRESH_GAUSSIAN_C, CV_THRESH_BINARY, 99, 10);
	
/*****************************************************************************************


  int count=0;
  double separation;
  double *hist = new double[region.dot_img->height];
  double var=0,ave=0;
  int linewid = 0;
  int up = 0;
  int down = 0;
  
  fill(hist,hist+region.dot_img->height,0);
  
  //黒画素のヒストグラムを作成
  for(int i=0;i<region.dot_img->height;i++){
    for(int j=0;j<region.dot_img->width;j++){
      if((int)((unsigned char)region.dot_img->imageData[region.dot_img->widthStep*i+j]) == 0){
	hist[i] += 1.0;
	count ++;
      }
    }
  }
  
  //確率分布を作成
  for(int i=0;i<region.dot_img->height;i++){
    hist[i] /= count;
    ave += hist[i]/region.dot_img->height;//進捗前
  }
  
  //進捗前
  for(int i=0;i<region.dot_img->height;i++){
    var += pow(hist[i]-ave,2) / region.dot_img->height;
  }
  //進捗前
  var = sqrt(var);

  //上下に黒画素が少なければ消す
  for(int i=0; i<region.dot_img->height; i++){
	  if(hist[i] < ave - var)  up++;
	  else break;
	}
	
	for(int i=region.dot_img->height; i>0; i--){
		if(hist[i] < ave - var) down++;
		else break;
	}
	
	cvSetImageROI(region.dot_img,cvRect(0, up, region.dot_img->width, region.dot_img->height-up-down));
	
	

//*********************************************************************************** */

//	region.dot_img = dst_img;
  //処理用画像にコピー
  region.seg_img = cvCloneImage(region.dot_img);
  
  //cvSaveImage("1.png",region.dot_img);
  
  //情報の受け渡し
  region.angle  = cinfo.angle;
  region.center = cinfo.center;
  
  for(int i=0;i<4;i++){
    region.R[i] = cinfo.R[i];
  }

}

void LoadCandidateRegion(region_t *region, class_info_t *cinfo, int class_num, IplImage *src_img, char *name){
  
  double x,y;
  char str[256];
  IplImage *dst_img;
  Point2f pt[3];
  CvMat *map_matrix;
  CvPoint2D32f src_pnt[3], dst_pnt[3];
  


  
  for(int i=0;i<class_num;i++){
    
    // (1)画像の読み込み，出力用画像領域の確保を行なう
    dst_img = cvCloneImage(src_img);
    
    // (2)三角形の回転前と回転後の対応する頂点をそれぞれセットしてcvGetAffineTransformを用いてアフィン行列を求める
    src_pnt[0] = cvPoint2D32f(cinfo[i].R[0].x, cinfo[i].R[0].y);
    src_pnt[1] = cvPoint2D32f(cinfo[i].R[1].x, cinfo[i].R[1].y);
    src_pnt[2] = cvPoint2D32f(cinfo[i].R[2].x, cinfo[i].R[2].y);
    
    //名前をつける
    sprintf(str, "TMP1/%s-%d.bmp", name, i);
    IFcout(2) << str << " " << cinfo[i].angle * 180 / PI << endl;
    
    for(int j=0;j<3;j++){
      x = cinfo[i].R[j].x - cinfo[i].center.x;
      y = cinfo[i].R[j].y - cinfo[i].center.y;
      pt[j].x = x*cos(-cinfo[i].angle) - y*sin(-cinfo[i].angle) + src_img->width/2;//範囲外アクセスをできるだけ減らすために
      pt[j].y = x*sin(-cinfo[i].angle) + y*cos(-cinfo[i].angle) + src_img->height/2;//centerではなく画像中央に置いておく
      IFcout(2) << "Rect " << j << " : " << cinfo[i].R[j] << endl;
    }
    IFcout(2) << endl;
    

    dst_pnt[0] = cvPoint2D32f (pt[0].x, pt[0].y);
    dst_pnt[1] = cvPoint2D32f (pt[1].x, pt[1].y);
    dst_pnt[2] = cvPoint2D32f (pt[2].x, pt[2].y);
    
    map_matrix = cvCreateMat (2, 3, CV_32FC1);
    cvGetAffineTransform (src_pnt, dst_pnt, map_matrix);
    
    //範囲外の矩形は修正 && 画像保存指定
    if(pt[0].x < 0) pt[0].x = 0;
    if(pt[1].x < 0) pt[1].x = 0;
    if(pt[2].x < 0) pt[2].x = 0;
    if(pt[0].y < 0) pt[0].y = 0;
    if(pt[1].y < 0) pt[1].y = 0;
    if(pt[2].y < 0) pt[2].y = 0; 
    if(pt[0].x > src_img->width-1)  pt[0].x = src_img->width-1;
    if(pt[1].x > src_img->width-1)  pt[1].x = src_img->width-1;
    if(pt[2].x > src_img->width-1)  pt[2].x = src_img->width-1;
    if(pt[0].y > src_img->height-1) pt[0].y = src_img->height-1;
    if(pt[1].y > src_img->height-1) pt[1].y = src_img->height-1;
    if(pt[2].y > src_img->height-1) pt[2].y = src_img->height-1;
    
    // (3)指定されたアフィン行列により，cvWarpAffineを用いて画像を回転させる。切れた部分は白にする。
    cvWarpAffine(src_img, dst_img, map_matrix, CV_INTER_LINEAR + CV_WARP_FILL_OUTLIERS, cvScalarAll(255));
    
    // (4)ROIを指定して保存
    cvSetImageROI(dst_img, cvRect(pt[1].x, pt[1].y, pt[2].x - pt[1].x, pt[0].y - pt[1].y));
    
    // (5)文字列が縦長の場合は90度回転して横長に補正して保存する
    if(pt[2].x - pt[1].x <  pt[0].y - pt[1].y){
      Rotate90save(dst_img, pt[1].x, pt[1].y, (int)(pt[2].x-pt[1].x), (int)(pt[0].y - pt[1].y), str);
    }
    else{
      cvSaveImage(str, dst_img);
    }
    
    //(6)領域の情報を設定する、二値画像に変換して読み込む（画像サイズ変更率：関数内で指定）
    setCandidateRegion(region[i], cinfo[i], str);
    sprintf(str, "TMP1/2/2%s-%d.bmp", name, i);
    cvSaveImage(str,region[i].seg_img);
  }
  
  cvReleaseImage (&dst_img);
  cvReleaseMat (&map_matrix);
  
}
