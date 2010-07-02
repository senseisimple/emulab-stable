#include <math.h>
#include <algorithm> // reverse_copy()
#include <numeric>   // accumulate()
#include "dc.hh"
#include <iostream> // DEBUG

const int MAX_SAMPLE = 1024;
const int TEST_DURATION = 100;
const int FILTER_LENGTH = 12; /* Using 'db6' wavelet */
const int MAX_LEVEL = 4; /* Maximum decomposition level */
const double LOG2 = 0.69314718055994529f; // log(2)
const double _SMALL = 1.0e-15f;

using namespace std;

//vector<double> wavelet_denoising(vector<double> );
//double xcor(vector<double> ,vector<double> );
//vector<double> wdec(vector<double>,int);
//vector<double> wconv(vector<double>, double *);
//vector<double> wrec(vector<double> ,vector<double>,int);

/* Filter coefficients for 'db6' wavelet */
double LD[12]={-0.00107730108500,0.00477725751101,0.00055384220099,-0.03158203931803,
    0.02752286553002,0.09750160558708,-0.12976686756710,-0.22626469396517,
    0.31525035170924,0.75113390802158,0.49462389039839,0.11154074335008};
double HD[12]={-0.11154074335008,0.49462389039839,-0.75113390802158,0.31525035170924,
    0.22626469396517,-0.12976686756710,-0.09750160558708,0.02752286553002,
    0.03158203931803,0.00055384220099,-0.00477725751101,-0.00107730108500};
double LR[12]={0.11154074335008,0.49462389039839,0.75113390802158,0.31525035170924,
    -0.22626469396517,-0.12976686756710,0.09750160558708,0.02752286553002,
    -0.03158203931803,0.00055384220099,0.00477725751101,-0.00107730108500};
double HR[12]={-0.00107730108500,-0.00477725751101,0.00055384220099,0.03158203931803,
    0.02752286553002,-0.09750160558708,-0.12976686756710,0.22626469396517,
    0.31525035170924,-0.75113390802158,0.49462389039839,-0.11154074335008};

vector<double> wconv(const vector<double> &data, double *filter)
{
  int lx = data.size();
  int lf = FILTER_LENGTH;
  vector<double> result;
  int i,j;
  double sum = 0;

    for(i=1;i<lx+lf;i++)
    {
        if(i<lf)
        {
            for(j=0;j<i;j++)
                sum=sum+filter[j]*data[i-j-1];
        }
        else if(i<lx+1)
        {
            for(j=0;j<lf;j++)
                sum=sum+filter[j]*data[i-j-1];
        }
        else
        {
            for(j=i-lx;j<lf;j++)
                sum=sum+filter[j]*data[i-j-1];
        }
        result.push_back(sum);
        sum=0;
    }
    
    return result;
}

vector<double> wdec(vector<double> data,int option)
{
    double diff;
    int len,i;
    vector<double> conv_result;
    vector<double> dyaddown;
    vector<double> edata;

    len=data.size();

    /* Symmetric expansion to prevent edge effect */
    reverse_copy(data.begin(),(data.begin()+FILTER_LENGTH-1),inserter(edata,edata.begin()));
    copy(data.begin(),data.end(),inserter(edata,edata.end()));
    reverse_copy((data.end()-FILTER_LENGTH+1),data.end(),inserter(edata,edata.end()));
    
    /* option 1 for detail space, option 2 for approximation space */
    if(option == 1)
        conv_result=wconv(edata,HD);
    else if(option == 2)
        conv_result=wconv(edata,LD);

    diff = (conv_result.size()-(len+FILTER_LENGTH-1))/2.0f;
    conv_result.erase((conv_result.end()-(int)floor(diff)),conv_result.end());
    conv_result.erase(conv_result.begin(),(conv_result.begin()+(int)ceil(diff)));

    /* Downsampling */
    vector<double>::iterator index;
    i=1;
    for(index=conv_result.begin();index!=conv_result.end();index++)
    {
        if((i++)%2 ==0)
            dyaddown.push_back(*(index));
    }
        
    return dyaddown;
}

vector<double> wrec(vector<double> app,vector<double> detail,int length)
{
    vector<double> app_up;
    vector<double> detail_up;
    vector<double> app_conv;
    vector<double> detail_conv;
    int i,diff;

    /* Upsampling */
    for(i=1;i<=2*(app.size());i++)
    {
        if(i%2==1)
        {
            app_up.push_back(0.0);
            detail_up.push_back(0.0);
        }
        else
        {
            app_up.push_back(app[(i/2)-1]);
            detail_up.push_back(detail[(i/2)-1]);
        }
    }
    
    app_conv=wconv(app_up,LR);
    detail_conv=wconv(detail_up,HR);

    /* Adding detail and approximation */
    for(i=0;i<app_conv.size();i++)
        app_conv[i]=app_conv[i] + detail_conv[i];
    
    app_conv.erase(app_conv.begin(),(app_conv.begin()+FILTER_LENGTH-1));
    diff=app_conv.size()-length;
    app_conv.erase((app_conv.end()-diff),app_conv.end());

    return app_conv;
}

vector<double> wavelet_denoising(const vector<double> &data, int nSamples)
{
    double median;
    int appr_size,next_size;
    int med_position;

    vector<double> wcoeffs;
    vector<double> detail;
    vector<double> appr;
    vector<double> abs_det;
    
    // Decide the maximum decomposition level.
    int nblev;
    nblev=(int)floorf(logf(nSamples)/LOG2 - logf(logf(nSamples))/LOG2);
    if(nblev > MAX_LEVEL) nblev = MAX_LEVEL;

    /* Deciding threshold value for 'MINIMAXI' scheme */
    int no_wcoeffs = nSamples; /* Add the size of approximation */
    int detail_size = nSamples;
    double thr, thr_lev;
    for(int i = 1; i <= nblev; i++) {
        detail_size= (int)floorf((detail_size+FILTER_LENGTH-1) / 2.0f);
        no_wcoeffs += detail_size;
    }
    if(no_wcoeffs <= 32) thr = 0;
    else thr = 0.3936 + 0.1829 * (logf(no_wcoeffs)/LOG2);

    /* Wavelets decomposition & thresholding */
    vector<int> wcolen;
    wcolen.push_back(nSamples);
    copy(data.begin(), data.end(), inserter(appr, appr.begin()));
    for(int i = 1; i <= nblev; i++) {
        detail = wdec(appr, 1);  /* Fine detail space */
        appr = wdec(appr, 2);    /* Coarse approximation space */
        abs_det.clear();
        transform(detail.begin(),detail.end(),inserter(abs_det,abs_det.begin()),fabsf);
        
        /* Finding the median to scale threshold */
        sort(abs_det.begin(),abs_det.end());
        if( (abs_det.size())%2 == 1 )
        {
            med_position=(int)floor(abs_det.size()/2.0f); 
            median = abs_det[med_position];
        }
        else
        {
            med_position=(int)(abs_det.size()/2);
            median =( abs_det[med_position-1] + abs_det[med_position] )/2.0;
        }

        thr_lev=thr*(median/0.6745);

        /* Soft-thresholding */
        for(int j=0;j<detail.size();j++)
        {
            if(detail[j] >= 0)
            {
                if(detail[j] <= thr_lev)
                    detail[j]=0;
                else
                    detail[j]=detail[j]-thr_lev;
            }
            else
            {
                if(detail[j] >= (-1*thr_lev) )
                    detail[j]=0;
                else
                    detail[j]=detail[j]+thr_lev;
            }
        }
                

        copy(detail.begin(),detail.end(),inserter(wcoeffs,wcoeffs.end()));
        wcolen.push_back(detail.size());
        
    }
    wcolen.push_back(appr.size());
    copy(appr.begin(),appr.end(),inserter(wcoeffs,wcoeffs.end()));
    
    /* Wavelets reconstruction */
    appr.clear();
    detail.clear();
    appr_size=wcolen.back();
    wcolen.pop_back();
    next_size=wcolen.back();
    wcolen.pop_back();
    copy((wcoeffs.end()-appr_size),wcoeffs.end(),inserter(appr,appr.begin()));
    wcoeffs.resize(wcoeffs.size()-appr_size);
    for(int i=1;i<=nblev;i++)
    {
        copy((wcoeffs.end()-next_size),wcoeffs.end(),inserter(detail,detail.begin()));
        wcoeffs.resize(wcoeffs.size()-next_size);
        next_size=wcolen.back();
        wcolen.pop_back();
        appr=wrec(appr,detail,next_size);
    }

    return appr;

}

bool delay_correlation(const vector<double> &delay1,
		       const vector<double> &delay2,
		       int nSamples, double threshold)
{
  /* Wavelets denoising */
  vector<double> wd1;
  vector<double> wd2;
  wd1=wavelet_denoising(delay1, nSamples);
  wd2=wavelet_denoising(delay2, nSamples);

  /* Get cross-correlation between wavelet-denoised data */
  double suma = 0, sum2a = 0, sumb = 0, sum2b = 0, xsum = 0;
  for (int i = 0; i < nSamples; i++) {
    suma += wd1[i];
    sum2a += wd1[i] * wd1[i];
    sumb += wd2[i];
    sum2b += wd2[i] * wd2[i];
    xsum += wd1[i] * wd2[i];
    //printf("Wd1[%d] = %f, Wd2[%d] = %f\n", i, wd1[i], i, wd2[i]);
    //printf("%f %f\n",wd1[i], wd2[i]);
  }
  double meana = suma / nSamples, meanb = sumb / nSamples;
  double sum = xsum - meana * sumb - meanb * suma + meana * meanb * nSamples;
  if (sum < _SMALL && sum > -_SMALL) sum = 0;
  //double varNa = sum2a - nSamples*meana*meana;
  //double varNb = sum2b - nSamples*meanb*meanb;
  double varNa = 0;
  double varNb = 0;
  sum = 0;

  for (int i = 0; i < nSamples; i++) {
      varNa += (wd1[i] - meana)*(wd1[i] - meana);
      varNb += (wd2[i] - meanb)*(wd2[i] - meanb);
  //    printf("Meana = %f, wd1[%d] = %f, varNa = %f, diff = %f\n", meana, i, wd1[i], varNa, wd1[i] - meana);

      sum += ((wd1[i] - meana)*(wd2[i] - meanb));
  }
  double xcor_value;
  if (sum == 0.0) {
    if (varNa < _SMALL && varNb < _SMALL) xcor_value = 0.998;
    if (varNa < _SMALL || varNb < _SMALL) xcor_value = 0.002;
  } else {
  //std::cout<<"sum2a = "<<sum2a<<", nSamples = "<<nSamples<<", meana = "<<meana<<", suma = "<<suma<<"\n";
  //std::cout<<"sum2b = "<<sum2b<<", nSamples = "<<nSamples<<", meanb = "<<meanb<<", sumb = "<<sumb<<"\n";
  std::cout<<"varNa = " <<varNa<<", varNb = "<<varNb << ", denom = "<<varNa*varNb<<"\n";
    xcor_value = sum / sqrt(varNa * varNb);
  }
//  if(varNa < 50.0 || varNb < 50.0)
 //   xcor_value = 0.0001;
  //std::cout<<"CORRELATION="<<xcor_value<<std::endl;
  printf("CORRELATION=%.3f\n", xcor_value);
  return (xcor_value >= threshold)? 1:0;
}
