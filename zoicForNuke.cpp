//ZOIC FOR NUKE
 
//TODO
// THROW ERROR WHEN FILTER TOO BIG
// ZDEPTH
// GRADIENT IN VALUES
 
 
kernel ZoicForNukeKernel : public ImageComputationKernel<ePixelWise>
{
    Image<eRead, eAccessRanged2D, eEdgeClamped> src;
    Image<eRead, eAccessRandom> srcFull;
    Image<eRead, eAccessRandom, eEdgeConstant> filter;
    Image<eRead, eAccessRandom> depth;
    Image<eWrite> dst;
 
local:
    int2 filterRadius;
    int2 _filterOffset;
    float _imageHypotenuse;
    int imageHeight;
    int imageWidth;
 
param:
    float opticalVignetting;
    float2 focus;
 
    void define(){
        defineParam(opticalVignetting, "Optical Vignetting", 0.0f);
        defineParam(focus, "Focal Point", float2(100.0f, 100.0f));
    }
 
    void init(){
        //Get the size of the filter input and store the radius.
        filterRadius = (filter.bounds.width()/2, filter.bounds.height()/2);
 
        //Store the offset of the bottom-left corner of the filter image
        //from the current pixel:
        _filterOffset[0] = (filter.bounds.x1 - filterRadius[0]);
        _filterOffset[1] = (filter.bounds.y1 - filterRadius[1]);
 
        //Set up the access for the src image
        src.setRange(-filterRadius[0], -filterRadius[1], filterRadius[0], filterRadius[1]);
 
        imageHeight = srcFull.bounds.height(); // THIS ISNT WORKING FOR SOME REASON?
        imageWidth = srcFull.bounds.width();
 
        // half of the diagonal of the image (center to corner length)
        _imageHypotenuse = sqrt(imageWidth * imageWidth + imageHeight * imageHeight) / 2.0;
 
    }
 
    void process(int2 pos) {
 
        //Get the depth at the current pixel
        float depthHere = depth(pos.x, pos.y, 0);
        //Get the depth at the focal point
        float depthAtFocus = bilinear(depth, focus[0], focus[1], 0);
        //Find the difference in depth between the current pixel and the focal point
        float z = fabs(depthHere - depthAtFocus);
        //Use the depth difference to set the blur size for the current pixel.
        // SHOULD BE CHANGED TO A DIAGONAL OR SOMETHING
        float blurSize = clamp(float((z * float(filter.bounds.x2) + 0.5f)), float(0.0), float(filter.bounds.x2));
        float blurSizePercentage = blurSize / (float)filter.bounds.width();
 
 
        SampleType(src) valueSum(0);
        ValueType(filter) filterSum(0);
 
 
        float distanceToCenterX = (pos.x - (imageWidth / 2));
        float distanceToCenterY = (pos.y - (imageHeight / 2));
 
        float percentageX = (distanceToCenterX / _imageHypotenuse);
        float percentageY = (distanceToCenterY / _imageHypotenuse);
 
        float shiftx = filterRadius.x * (percentageX * opticalVignetting);
        float shifty = filterRadius.y * (percentageY * opticalVignetting);
 
        float newFilterSizeY = filter.bounds.y2 * blurSizePercentage;
        float newFilterSizeX = filter.bounds.x2 * blurSizePercentage;
 
        //Iterate over the filter image
        for(int j = 0; j < int(newFilterSizeY); j++) {
            for(int i = 0; i < int(newFilterSizeX); i++) {
            
                //Get the filter value
                ValueType(filter) filterVal = filter(i, j, 0);
 
                // multiply filter by another offset virtual filter
                filterVal *= bilinear(filter, float(i) - shiftx, float(j) - shifty, 0);
 
                // Multiply the src value by the corresponding filter weight and accumulate
                valueSum += filterVal * bilinear(src, i + _filterOffset[0] * blurSizePercentage, j + _filterOffset[0] * blurSizePercentage);
               
 
               
                // Accumulate filter values
                filterSum += filterVal;
 
 
            }
        }
 
        // Normalise the value sum, avoiding division by zero
        if (filterSum != 0){
            valueSum /= filterSum;
        } else {
            //valueSum = srcFull(pos.x, pos.y);
        }
 
        dst() = valueSum;
 
       
    }
};