#include "SubHeader.h"

//正規化ヒストグラムの判別分析法
int ArrayThreshold(double *hist, int num){
  
  int threshold = 0;
  double ave  = 0;
  double var_b = 0.0, varmax_b = 0.0;
  double *m1  = new double [num];
  double *m2  = new double [num];
  double *om1 = new double [num];
  double *om2 = new double [num];
  
  //確率分布の全体平均
  for(int i=0;i<num;i++){
    ave += i*hist[i];
  }
  
  m1[0]  = ave;
  m2[0]  = 0.0;
  om1[0] = 1.0;
  om2[0] = 0.0;
  
  for( int i = 1; i < num; i++ ){
    
    om2[i] = om2[i-1] + hist[i-1];
    om1[i] = 1.0 - om2[i];
    m2[i] = om2[i] > 0 ? ( m2[i-1]*om2[i-1] + (i-1)*hist[i-1] ) / om2[i] : 0;
    m1[i] = om1[i] > 0 ? ( m1[0] - om2[i]*m2[i]) / om1[i] : 0;
    var_b = om1[i]*om2[i]*(m1[i]-m2[i])*(m1[i]-m2[i]);
    
    if(varmax_b < var_b){
      threshold = i;
      varmax_b = var_b;
    }
    
  }
  
  delete [] m1;
  delete [] m2;
  delete [] om1;
  delete [] om2;
  
  return threshold;
  
}

//行間の位置を水平投影ヒストグラムの判別分析法で推定
void LineDetection(IplImage *img, int &line){
  
  int count=0;
  double separation;
  double *hist = new double[img->height];
  double var=0,ave=0;
  int linewid = 0;
  
  fill(hist,hist+img->height,0);
  
  //黒画素のヒストグラムを作成
  for(int i=0;i<img->height;i++){
    for(int j=0;j<img->width;j++){
      if((int)((unsigned char)img->imageData[img->widthStep*i+j]) == 0){
	hist[i] += 1.0;
	count ++;
      }
    }
  }
  
  //確率分布を作成
  for(int i=0;i<img->height;i++){
    hist[i] /= count;
    ave += hist[i]/img->height;//進捗前
  }
  
  //進捗前
  for(int i=0;i<img->height;i++){
    var += pow(hist[i]-ave,2) / img->height;
  }
  //進捗前
  var = sqrt(var);

  //判別分析で行の位置を推定
  line = ArrayThreshold(hist,img->height);//進捗前
  
  //十分に黒画素が存在するならば行ではない
  if(hist[line] > ave - var)  line = 0;
  else {
  	//幅の狭い方の6分の１程度
  	if(line > img->height/2.0)	linewid = (img->height-line)/6.0;
 		else	linewid = line/6.0;
/* 		
 		for(int i=0; i<img->width; i++){
 			int wid = 1;
			int up = 0;										
			int down = 0;
			
 			for(int j=0; j<linewid; j++){
 				if(up < 3 && (int)((unsigned char)img->imageData[img->widthStep*(j+line)+i])==1){	wid++;	}
 				else{ up++;	}
 				if(down < 3 && (int)((unsigned char)img->imageData[img->widthStep*(j+line)+i])==1){	wid++;	}
 				else{	down++;	}
 			}
 		if(wid < linewid){
 			line = 0;
 			break;
 		}
 		}
*/
		int wid = 1;
		int up = 0;
		int down = 0;

 	 	for(int i=0; i<linewid; i++){
  		if(up < 1 && hist[line-i] < ave - var){	wid++; }
  		else{	up++;	}
  		if(down < 1 && hist[line+i] < ave - var){	wid++;	}
  		else{	down++;	}
  	}
  	if(wid < linewid) line = 0;
  }
  delete[] hist;
  
}

void VerticalDilation(IplImage *img, int line){
  
  //CV             
  IplConvKernel *element1,*element2;
  int kernel1[9]= {0,1,0,
		   0,1,0,
		   0,1,0};
  int kernel2[9]= {1,1,1,
		   1,1,1,
		   1,1,1};
  
  //cvSaveImage("nondilate.png",img);
  element1 = cvCreateStructuringElementEx(3,3,1,1,CV_SHAPE_CUSTOM,kernel1);
  element2 = cvCreateStructuringElementEx(3,3,1,1,CV_SHAPE_CUSTOM,kernel2);
  cvErode(img,img,element1,12);
  cvErode(img,img,element2,3);
  //cvSaveImage("dilate.png",img);
  
  if(line > 0){
    for(int i=0;i<img->width;i++){
      img->imageData[img->widthStep*line+i] = 0xFF;
    }
  }
  
}


void BlackPixelDilation(IplImage *img, int line){

  IplImage *tmp;  

  for(int p=0;p<15;p++){
    
    tmp = cvCloneImage(img);  
    
    for(int i=1;i<tmp->height-1;i++){
      for(int j=1;j<tmp->width-1;j++){

	if((int)((unsigned char)tmp->imageData[tmp->widthStep*i+j]) == 0x00){
	  
	  if(i+1 != line)
	    img->imageData[tmp->widthStep*(i+1)+j] = 0x00;
	  if(i-1 != line)
	    img->imageData[tmp->widthStep*(i-1)+j] = 0x00;
	  
	  if(p >= 12){
	    img->imageData[tmp->widthStep*i+j-1] = 0x00;
	    img->imageData[tmp->widthStep*i+j+1] = 0x00;
	    if(i+1 != line){
	      img->imageData[tmp->widthStep*(i+1)+j-1] = 0x00;
	      img->imageData[tmp->widthStep*(i+1)+j+1] = 0x00;
	    }
	    if(i-1 != line){
	      img->imageData[tmp->widthStep*(i-1)+j-1] = 0x00;
	      img->imageData[tmp->widthStep*(i-1)+j+1] = 0x00;
	    }
	  }
	  
	}
	
      }
    }
    
  }
  
}

void SegmentationPreprocess(region_t *region, int class_num){
  
#ifdef _OPENMP
#pragma omp parallel for
#endif

  for(int i=0;i<class_num;i++){
    
    LineDetection(region[i].seg_img, region[i].line);

    //BlackPixelDilation(region[i].seg_img, region[i].line);
    
    VerticalDilation(region[i].seg_img, region[i].line);
    
  }
  
}
