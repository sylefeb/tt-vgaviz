
#include <LibSL/LibSL.h>
#include <LibSL/LibSL_gl.h>

#include <thread>
#include <mutex>

#include "VgaChip.h"
#include "VCDParser.h"
#include "fstapi.h"

// ---------------------------------------------------------------------

// AutoPtr<VCDParser> g_VCD;
VgaChip            g_VGA;
Tex2DRGBA_Ptr      g_Tex;
void              *g_Wave = nullptr;

std::unordered_map<fstHandle, std::string> g_HandleToName;
std::unordered_map<std::string, uint64_t>  g_Values;

std::mutex         g_Mutex;

bool               g_Done = false;

// ---------------------------------------------------------------------

void main_render()
{
  {
    std::unique_lock<std::mutex> lock(g_Mutex);

    if (g_VGA.frameBufferChanged()) {
      glClearColor(0, 0, 1, 0);
      glClear(GL_COLOR_BUFFER_BIT);

      g_Tex = Tex2DRGBA_Ptr(new Tex2DRGBA(g_VGA.frameBuffer()->pixels(), GPUTEX_AUTOGEN_MIPMAP));

      glBindTexture(GL_TEXTURE_2D, g_Tex->handle());
      glEnable(GL_TEXTURE_2D);
      LibSL::GPUHelpers::Transform::ortho2D(LIBSL_PROJECTION_MATRIX, 0.0f, 1.0f, 1.0f, 0.0f);
      LibSL::GPUHelpers::Transform::identity(LIBSL_MODELVIEW_MATRIX);
      glColor3f(1, 1, 1);
      glBegin(GL_QUADS);
      glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
      glTexCoord2f(1.0f, 0.0f); glVertex2f(1.0f, 0.0f);
      glTexCoord2f(1.0f, 1.0f); glVertex2f(1.0f, 1.0f);
      glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, 1.0f);
      glEnd();
    }
  }

  // std::this_thread::yield();
  std::this_thread::sleep_for(std::chrono::milliseconds(150));

  if (g_Done) {
    SimpleUI::exit();
  }
  
}

// ---------------------------------------------------------------------

uint64_t decodeValue(const char *str)
{
  uint64_t val = 0;
  while (*str != '\0') {
    val = (val << 1) | (*str == '1' ? 1 : 0);
    str++;
  }
  return val;
}

// ---------------------------------------------------------------------

void value_change_callback(void* user_callback_data_pointer, uint64_t time, fstHandle facidx, const unsigned char* value)
{
  std::unique_lock<std::mutex> lock(g_Mutex);

  g_Values[g_HandleToName[facidx]] = decodeValue((const char*)value);

  static bool prev_clk  = false;
  static int  clk_count = 0;
  bool clk = g_Values["clk"];
  if (clk != prev_clk) {
    prev_clk = clk;
    if ((clk_count & 1023) == 0) {
      std::cerr << '.';
    }
    ++ clk_count;
    uint8_t uo_out = g_Values["uo_out [7:0]"];
    g_VGA.step(
      clk,
      (uint8_t)((uo_out>>3)&1), //vs
      (uint8_t)((uo_out>>7)&1), //hs
      (uint8_t)(
        (((uo_out>>0)&1)<<1)
       |(((uo_out>>4)&1)<<0)
      ),  //r
      (uint8_t)(
        (((uo_out>>1)&1)<<1)
       |(((uo_out>>5)&1)<<0)
      ),  //g
      (uint8_t)(
        (((uo_out>>2)&1)<<1)
       |(((uo_out>>6)&1)<<0)
      )   //b
    );

    std::this_thread::yield();
  }

  // std::cerr << g_HandleToName[facidx] << " = " << decodeValue((const char* )value) << std::endl;
}

// ---------------------------------------------------------------------

int main(int argc,char **argv)
{
  try {

    if (argc < 2) {
      std::cerr << "Please provide fst wave file as argument" << std::endl;
      return -1;
    }
    if (!LibSL::System::File::exists(argv[1])) {
      std::cerr << "Could not open fst wave file " << argv[1] << std::endl;
      return -1;
    }

    SimpleUI::init(g_VGA.w(), g_VGA.h(), "VGA Trace Visualizer");

    SimpleUI::onRender = main_render;

    g_Wave = fstReaderOpen(argv[1]);

    struct fstHier* hier = fstReaderIterateHier(g_Wave);
    do {
      switch (hier->htyp) {
      case FST_HT_SCOPE:
        std::cerr << "scope : " << hier->u.scope.name << std::endl;
        break;
      case FST_HT_ATTRBEGIN:
        // std::cerr << "attr  : " << hier->u.attr.name << std::endl;
        break;
      case FST_HT_VAR:
        std::cerr << "signal: " << hier->u.var.name << std::endl;
        g_HandleToName.insert(std::make_pair(hier->u.var.handle, hier->u.var.name));
        break;
      default:
        // std::cerr << "unkonwn" << std::endl;
        break;
      }
      hier = fstReaderIterateHier(g_Wave);
    } while (hier != NULL);

    fstReaderSetFacProcessMaskAll(g_Wave);

    std::thread th([](){
      fstReaderIterBlocks(g_Wave, value_change_callback, NULL, NULL);
      g_Done = true;
    });

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);

    std::cerr << "Please wait, it can take a while (alive while dots appear) ";

    SimpleUI::loop();

    g_Tex = Tex2DRGBA_Ptr();

    SimpleUI::shutdown();

    std::cerr << '\n';

  } catch (Fatal& f) {
    std::cerr << Console::red << "[fatal] " << f.message() << Console::gray << std::endl;
  }
  return -1;
}

// ---------------------------------------------------------------------

