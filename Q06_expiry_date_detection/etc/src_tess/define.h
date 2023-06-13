/* Model Binary */
const static std::string model_dir = "date_tinyyolov3_onnx";
/* Pre-processing Runtime Object */
const static std::string pre_dir = model_dir + "/preprocess";

/* Class labels to be classified */
const static std::string label_list = "date_class_labels.txt";

/* Empty List since labels will be loaded from label_list file */
static std::vector<std::string> label_file_map = {};

/*****************************************
 * Macro for YOLOv3
 ******************************************/

/* Number of class to be detected */
// #define NUM_CLASS                   (80)
#define NUM_CLASS (4)
/* Number for [region] layer num parameter */
#define NUM_BB (3)
#define NUM_INF_OUT_LAYER (2)
/* Thresholds */
#define TH_PROB (0.2f)
#define TH_NMS (0.5f)
/* Size of input image to the model */
#define MODEL_IN_W (416)
#define MODEL_IN_H (416)

/* Number of grids in the image. The length of this array MUST match with the NUM_INF_OUT_LAYER */
const static uint8_t num_grids[] = {13, 26};
/* Number of DRP-AI output */
const static uint32_t INF_OUT_SIZE = (NUM_CLASS + 5) * NUM_BB * num_grids[0] * num_grids[0] + (NUM_CLASS + 5) * NUM_BB * num_grids[1] * num_grids[1];


const static double anchors[] =
    {
        21, 12,
        57, 11,
        104, 15,
        62, 27,
        146, 28,
        206, 47
    };


/****************************************
 * Structure to store the date variable
 *****************************************/
struct date_struct
{
    std::string txt_extr;
    std::string year;
    std::string month;
    std::string day;
};

#define MIN_CROP_HEIGHT             (84)
#define TESS_IMG_RESOLUTION         (330)
#define CROP_IMG_STRETCH            (5) // Pixel value Used to extended crop section from the image


/*Image:: Text information to be drawn on image*/
#define CHAR_SCALE_LARGE            (0.8)
#define CHAR_SCALE_SMALL            (0.7)
#define CHAR_THICKNESS              (2)
#define LINE_HEIGHT                 (30) /*in pixel*/
#define LINE_HEIGHT_OFFSET          (20) /*in pixel*/
#define WHITE_DATA                  (0xFFFFFFu) /* in RGB */


/*DRP-AI Input image information*/
// #define DRPAI_IN_WIDTH              (CAM_IMAGE_WIDTH)
// #define DRPAI_IN_HEIGHT             (CAM_IMAGE_HEIGHT)