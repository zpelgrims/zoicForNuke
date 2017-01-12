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
    float2 filterRadius;
    float2 _filterOffset;
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
        filterRadius = (float(filter.bounds.width()) * 0.5, float(filter.bounds.height()) * 0.5);

        //Store the offset of the bottom-left corner of the filter image
        //from the current pixel:
        _filterOffset[0] = (float(filter.bounds.x1) - filterRadius[0]);
        _filterOffset[1] = (float(filter.bounds.y1) - filterRadius[1]);

        //Set up the access for the src image
        src.setRange(-filterRadius[0], -filterRadius[1], filterRadius[0], filterRadius[1]);

        imageHeight = srcFull.bounds.height(); // THIS ISNT WORKING FOR SOME REASON?
        imageWidth = srcFull.bounds.width();

        // half of the diagonal of the image (center to corner length)
        _imageHypotenuse = sqrt(imageWidth * imageWidth + imageHeight * imageHeight) * 0.5;

    }


    float4 linearInterpolate(float perc, float4 a, float4 b){
        return a + perc * (b - a);
    }


    void process(int2 pos) {



        // Get the depth at the current pixel
        float depthHere = depth(pos.x, pos.y, 0);

        // Get the depth at the focal point
        float depthAtFocus = bilinear(depth, focus[0], focus[1], 0);
        
        // Find the difference in depth between the current pixel and the focal point
        float z = fabs(depthHere - depthAtFocus);

        // Use the depth difference to set the blur size for the current pixel.
        float blurSize = clamp(float((z * float(filter.bounds.x2))), float(0.0), float(filter.bounds.x2));
        float blurSizePercentage = blurSize / (float)filter.bounds.width();




        float focalLength = 35.0;
        float fstop = 1.4;
        float circleOfConfusion = (fabs(depthHere - depthAtFocus) / depthHere) * ((focalLength * focalLength) / (fstop * (depthAtFocus - focalLength)));





        SampleType(src) valueSum1(0);
        SampleType(src) valueSum2(0);
        ValueType(filter) filterSum1(0);
        ValueType(filter) filterSum2(0);


        float distanceToCenterX = (float(pos.x) - (float(imageWidth) * 0.5));
        float distanceToCenterY = (float(pos.y) - (float(imageHeight) * 0.5));

        float percentageX = (distanceToCenterX / _imageHypotenuse);
        float percentageY = (distanceToCenterY / _imageHypotenuse);

        float shiftx = filterRadius.x * (percentageX * opticalVignetting);
        float shifty = filterRadius.y * (percentageY * opticalVignetting);

        float newFilterSizeY = float(filter.bounds.y2) * blurSizePercentage;
        float newFilterSizeX = float(filter.bounds.x2) * blurSizePercentage;

        float coc = float(filter.bounds.x2) * blurSizePercentage;
        float cocPercentage = (coc - int(coc)) / (int(coc + 1.0) - int(coc));


        //Iterate over the filter image
        for(int j = 0; j < int(coc); j++) {
            for(int i = 0; i < int(coc); i++) { 
            
                //Get the filter value
                ValueType(filter) filterVal1 = filter(i, j, 0);

                // multiply filter by another offset virtual filter
                filterVal1 *= bilinear(filter, float(i) - shiftx, float(j) - shifty, 0);

                // Multiply the src value by the corresponding filter weight and accumulate
                valueSum1 += filterVal1 * bilinear(src, i + _filterOffset[0] * blurSizePercentage, j + _filterOffset[0] * blurSizePercentage);
                
                // Accumulate filter values
                filterSum1 += filterVal1;
            }
        }

        // Normalise the value sum
        valueSum1 = (filterSum1 > 1.0) ? valueSum1 / filterSum1 : srcFull(pos.x, pos.y);


        //Iterate over the filter image
        for(int j = 0; j < int(coc + 1); j++) {
            for(int i = 0; i < int(coc + 1); i++) { 
            
                //Get the filter value
                ValueType(filter) filterVal2 = filter(i, j, 0);

                // multiply filter by another offset virtual filter
                filterVal2 *= bilinear(filter, float(i) - shiftx, float(j) - shifty, 0);

                // Multiply the src value by the corresponding filter weight and accumulate
                valueSum2 += filterVal2 * bilinear(src, i + _filterOffset[0] * blurSizePercentage, j + _filterOffset[0] * blurSizePercentage);
                
                // Accumulate filter values
                filterSum2 += filterVal2;
            }
        }

        // Normalise the value sum
        valueSum2 = (filterSum2 > 1.0) ? valueSum2 / filterSum2 : srcFull(pos.x, pos.y);
        


        //dst() = linearInterpolate(cocPercentage, valueSum1, valueSum2);
        dst() = circleOfConfusion;
    }
};