package org.libsdl.app;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.egl.*;

import android.app.*;
import android.content.*;
import android.view.*;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputMethodManager;
import android.widget.AbsoluteLayout;
import android.os.*;
import android.util.Log;
import android.graphics.*;
import android.text.method.*;
import android.text.*;
import android.media.*;
import android.hardware.*;
import android.content.*;

import java.lang.*;


/**
    SDL Activity
*/
public class SDLActivity extends Activity {

    // Keep track of the paused state
    public static boolean mIsPaused;

    // Main components
    private static SDLActivity mSingleton;
    private static SDLSurface mSurface;
    private static View mTextEdit;
    private static ViewGroup mLayout;

    // This is what SDL runs in. It invokes SDL_main(), eventually
    private static Thread mSDLThread;

    // Audio
    private static Thread mAudioThread;
    private static AudioTrack mAudioTrack;

    // EGL private objects
    private static EGLContext  mEGLContext;
    private static EGLSurface  mEGLSurface;
    private static EGLDisplay  mEGLDisplay;
    private static EGLConfig   mEGLConfig;
    private static int mGLMajor, mGLMinor;

    // Load the .so
    static {
        System.loadLibrary("sdl2");
        System.loadLibrary("openbor");
    }

    // Setup
    protected void onCreate(Bundle savedInstanceState) {
        //Log.v("SDL", "onCreate()");
        super.onCreate(savedInstanceState);
        
        // So we can call stuff from static callbacks
        mSingleton = this;

        // Keep track of the paused state
        mIsPaused = false;

        // Set up the surface
        mSurface = new SDLSurface(getApplication());

        mLayout = new AbsoluteLayout(this);
        mLayout.addView(mSurface);

        setContentView(mLayout);

        SurfaceHolder holder = mSurface.getHolder();
    }

    // Events
    /*protected void onPause() {
        Log.v("SDL", "onPause()");
        super.onPause();
        // Don't call SDLActivity.nativePause(); here, it will be called by SDLSurface::surfaceDestroyed
    }

    protected void onResume() {
        Log.v("SDL", "onResume()");
        super.onResume();
        // Don't call SDLActivity.nativeResume(); here, it will be called via SDLSurface::surfaceChanged->SDLActivity::startApp
    }*/

    protected void onDestroy() {
        super.onDestroy();
        Log.v("SDL", "onDestroy()");
        // Send a quit message to the application
        SDLActivity.nativeQuit();

        // Now wait for the SDL thread to quit
        if (mSDLThread != null) {
            try {
                mSDLThread.join();
            } catch(Exception e) {
                Log.v("SDL", "Problem stopping thread: " + e);
            }
            mSDLThread = null;

            //Log.v("SDL", "Finished waiting for SDL thread");
        }
    }

    // Messages from the SDLMain thread
    static final int COMMAND_CHANGE_TITLE = 1;
    static final int COMMAND_UNUSED = 2;
    static final int COMMAND_TEXTEDIT_HIDE = 3;

    // Handler for the messages
    Handler commandHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.arg1) {
            case COMMAND_CHANGE_TITLE:
                setTitle((String)msg.obj);
                break;
            case COMMAND_TEXTEDIT_HIDE:
                if (mTextEdit != null) {
                    mTextEdit.setVisibility(View.GONE);

                    InputMethodManager imm = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
                    imm.hideSoftInputFromWindow(mTextEdit.getWindowToken(), 0);
                }
                break;
            }
        }
    };

    // Send a message from the SDLMain thread
    void sendCommand(int command, Object data) {
        Message msg = commandHandler.obtainMessage();
        msg.arg1 = command;
        msg.obj = data;
        commandHandler.sendMessage(msg);
    }

    // C functions we call
    public static native void nativeInit();
    public static native void nativeQuit();
    public static native void nativePause();
    public static native void nativeResume();
    public static native void onNativeResize(int x, int y, int format);
    public static native void onNativeKeyDown(int keycode);
    public static native void onNativeKeyUp(int keycode);
    public static native void onNativeTouch(int touchDevId, int pointerFingerId,
                                            int action, float x, 
                                            float y, float p);
    public static native void onNativeAccel(float x, float y, float z);
    public static native void nativeRunAudioThread();
    public static native void nativeUpdateTouchStates(float[] px, float[] py, int[] pid, int maxp);


    // Java functions called from C

    public static boolean createGLContext(int majorVersion, int minorVersion) {
        return initEGL(majorVersion, minorVersion);
    }

    public static void flipBuffers() {
        flipEGL();
    }

    public static void setActivityTitle(String title) {
        // Called from SDLMain() thread and can't directly affect the view
        mSingleton.sendCommand(COMMAND_CHANGE_TITLE, title);
    }

    public static void sendMessage(int command, int param) {
        mSingleton.sendCommand(command, Integer.valueOf(param));
    }

    public static Context getContext() {
        return mSingleton;
    }

    public static void startApp() {
        // Start up the C app thread
        if (mSDLThread == null) {
            mSDLThread = new Thread(new SDLMain(), "SDLThread");
            mSDLThread.start();
        }
        else {
            /*
             * Some Android variants may send multiple surfaceChanged events, so we don't need to resume every time
             * every time we get one of those events, only if it comes after surfaceDestroyed
             */
            if (mIsPaused) {
                SDLActivity.nativeResume();
                SDLActivity.mIsPaused = false;
            }
        }
    }
    
    static class ShowTextInputHandler implements Runnable {
        /*
         * This is used to regulate the pan&scan method to have some offset from
         * the bottom edge of the input region and the top edge of an input
         * method (soft keyboard)
         */
        static final int HEIGHT_PADDING = 15;

        public int x, y, w, h;

        public ShowTextInputHandler(int x, int y, int w, int h) {
            this.x = x;
            this.y = y;
            this.w = w;
            this.h = h;
        }

        public void run() {
            AbsoluteLayout.LayoutParams params = new AbsoluteLayout.LayoutParams(
                    w, h + HEIGHT_PADDING, x, y);

            if (mTextEdit == null) {
                mTextEdit = new DummyEdit(getContext());

                mLayout.addView(mTextEdit, params);
            } else {
                mTextEdit.setLayoutParams(params);
            }

            mTextEdit.setVisibility(View.VISIBLE);
            mTextEdit.requestFocus();

            InputMethodManager imm = (InputMethodManager) getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
            imm.showSoftInput(mTextEdit, 0);
        }

    }

    public static void showTextInput(int x, int y, int w, int h) {
        // Transfer the task to the main thread as a Runnable
        mSingleton.commandHandler.post(new ShowTextInputHandler(x, y, w, h));
    }


    // EGL functions
    public static boolean initEGL(int majorVersion, int minorVersion) {
        try {
            if (SDLActivity.mEGLDisplay == null) {
                Log.v("SDL", "Starting up OpenGL ES " + majorVersion + "." + minorVersion);

                EGL10 egl = (EGL10)EGLContext.getEGL();

                EGLDisplay dpy = egl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);

                int[] version = new int[2];
                egl.eglInitialize(dpy, version);

                int EGL_OPENGL_ES_BIT = 1;
                int EGL_OPENGL_ES2_BIT = 4;
                int renderableType = 0;
                if (majorVersion == 2) {
                    renderableType = EGL_OPENGL_ES2_BIT;
                } else if (majorVersion == 1) {
                    renderableType = EGL_OPENGL_ES_BIT;
                }
                int[] configSpec = {
                    //EGL10.EGL_DEPTH_SIZE,   16,
                    EGL10.EGL_RENDERABLE_TYPE, renderableType,
                    EGL10.EGL_NONE
                };
                EGLConfig[] configs = new EGLConfig[1];
                int[] num_config = new int[1];
                if (!egl.eglChooseConfig(dpy, configSpec, configs, 1, num_config) || num_config[0] == 0) {
                    Log.e("SDL", "No EGL config available");
                    return false;
                }
                EGLConfig config = configs[0];

                SDLActivity.mEGLDisplay = dpy;
                SDLActivity.mEGLConfig = config;
                SDLActivity.mGLMajor = majorVersion;
                SDLActivity.mGLMinor = minorVersion;
            }
            return SDLActivity.createEGLSurface();

        } catch(Exception e) {
            Log.v("SDL", e + "");
            for (StackTraceElement s : e.getStackTrace()) {
                Log.v("SDL", s.toString());
            }
            return false;
        }
    }

    public static boolean createEGLContext() {
        EGL10 egl = (EGL10)EGLContext.getEGL();
        int EGL_CONTEXT_CLIENT_VERSION=0x3098;
        int contextAttrs[] = new int[] { EGL_CONTEXT_CLIENT_VERSION, SDLActivity.mGLMajor, EGL10.EGL_NONE };
        SDLActivity.mEGLContext = egl.eglCreateContext(SDLActivity.mEGLDisplay, SDLActivity.mEGLConfig, EGL10.EGL_NO_CONTEXT, contextAttrs);
        if (SDLActivity.mEGLContext == EGL10.EGL_NO_CONTEXT) {
            Log.e("SDL", "Couldn't create context");
            return false;
        }
        return true;
    }

    public static boolean createEGLSurface() {
        if (SDLActivity.mEGLDisplay != null && SDLActivity.mEGLConfig != null) {
            EGL10 egl = (EGL10)EGLContext.getEGL();
            if (SDLActivity.mEGLContext == null) createEGLContext();

            Log.v("SDL", "Creating new EGL Surface");
            EGLSurface surface = egl.eglCreateWindowSurface(SDLActivity.mEGLDisplay, SDLActivity.mEGLConfig, SDLActivity.mSurface, null);
            if (surface == EGL10.EGL_NO_SURFACE) {
                Log.e("SDL", "Couldn't create surface");
                return false;
            }

            if (egl.eglGetCurrentContext() != SDLActivity.mEGLContext) {
                if (!egl.eglMakeCurrent(SDLActivity.mEGLDisplay, surface, surface, SDLActivity.mEGLContext)) {
                    Log.e("SDL", "Old EGL Context doesnt work, trying with a new one");
                    // TODO: Notify the user via a message that the old context could not be restored, and that textures need to be manually restored.
                    createEGLContext();
                    if (!egl.eglMakeCurrent(SDLActivity.mEGLDisplay, surface, surface, SDLActivity.mEGLContext)) {
                        Log.e("SDL", "Failed making EGL Context current");
                        return false;
                    }
                }
            }
            SDLActivity.mEGLSurface = surface;
            return true;
        } else {
            Log.e("SDL", "Surface creation failed, display = " + SDLActivity.mEGLDisplay + ", config = " + SDLActivity.mEGLConfig);
            return false;
        }
    }

    // EGL buffer flip
    public static void flipEGL() {
        try {
            EGL10 egl = (EGL10)EGLContext.getEGL();

            egl.eglWaitNative(EGL10.EGL_CORE_NATIVE_ENGINE, null);

            // drawing here

            egl.eglWaitGL();

            egl.eglSwapBuffers(SDLActivity.mEGLDisplay, SDLActivity.mEGLSurface);


        } catch(Exception e) {
            Log.v("SDL", "flipEGL(): " + e);
            for (StackTraceElement s : e.getStackTrace()) {
                Log.v("SDL", s.toString());
            }
        }
    }

    // Audio
    private static Object buf;
    
    public static Object audioInit(int sampleRate, boolean is16Bit, boolean isStereo, int desiredFrames) {
        int channelConfig = isStereo ? AudioFormat.CHANNEL_CONFIGURATION_STEREO : AudioFormat.CHANNEL_CONFIGURATION_MONO;
        int audioFormat = is16Bit ? AudioFormat.ENCODING_PCM_16BIT : AudioFormat.ENCODING_PCM_8BIT;
        int frameSize = (isStereo ? 2 : 1) * (is16Bit ? 2 : 1);
        
        Log.v("SDL", "SDL audio: wanted " + (isStereo ? "stereo" : "mono") + " " + (is16Bit ? "16-bit" : "8-bit") + " " + ((float)sampleRate / 1000f) + "kHz, " + desiredFrames + " frames buffer");
        
        // Let the user pick a larger buffer if they really want -- but ye
        // gods they probably shouldn't, the minimums are horrifyingly high
        // latency already
        desiredFrames = Math.max(desiredFrames, (AudioTrack.getMinBufferSize(sampleRate, channelConfig, audioFormat) + frameSize - 1) / frameSize);
        
        mAudioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, sampleRate,
                channelConfig, audioFormat, desiredFrames * frameSize, AudioTrack.MODE_STREAM);
        
        audioStartThread();
        
        Log.v("SDL", "SDL audio: got " + ((mAudioTrack.getChannelCount() >= 2) ? "stereo" : "mono") + " " + ((mAudioTrack.getAudioFormat() == AudioFormat.ENCODING_PCM_16BIT) ? "16-bit" : "8-bit") + " " + ((float)mAudioTrack.getSampleRate() / 1000f) + "kHz, " + desiredFrames + " frames buffer");
        
        if (is16Bit) {
            buf = new short[desiredFrames * (isStereo ? 2 : 1)];
        } else {
            buf = new byte[desiredFrames * (isStereo ? 2 : 1)]; 
        }
        return buf;
    }
    
    public static void audioStartThread() {
        mAudioThread = new Thread(new Runnable() {
            public void run() {
                mAudioTrack.play();
                nativeRunAudioThread();
            }
        });
        
        // I'd take REALTIME if I could get it!
        mAudioThread.setPriority(Thread.MAX_PRIORITY);
        mAudioThread.start();
    }
    
    public static void audioWriteShortBuffer(short[] buffer) {
        for (int i = 0; i < buffer.length; ) {
            int result = mAudioTrack.write(buffer, i, buffer.length - i);
            if (result > 0) {
                i += result;
            } else if (result == 0) {
                try {
                    Thread.sleep(1);
                } catch(InterruptedException e) {
                    // Nom nom
                }
            } else {
                Log.w("SDL", "SDL audio: error return from write(short)");
                return;
            }
        }
    }
    
    public static void audioWriteByteBuffer(byte[] buffer) {
        for (int i = 0; i < buffer.length; ) {
            int result = mAudioTrack.write(buffer, i, buffer.length - i);
            if (result > 0) {
                i += result;
            } else if (result == 0) {
                try {
                    Thread.sleep(1);
                } catch(InterruptedException e) {
                    // Nom nom
                }
            } else {
                Log.w("SDL", "SDL audio: error return from write(short)");
                return;
            }
        }
    }

    public static void audioQuit() {
        if (mAudioThread != null) {
            try {
                mAudioThread.join();
            } catch(Exception e) {
                Log.v("SDL", "Problem stopping audio thread: " + e);
            }
            mAudioThread = null;

            //Log.v("SDL", "Finished waiting for audio thread");
        }

        if (mAudioTrack != null) {
            mAudioTrack.stop();
            mAudioTrack = null;
        }
    }
}

/**
    Simple nativeInit() runnable
*/
class SDLMain implements Runnable {
    public void run() {
        // Runs SDL_main()
        SDLActivity.nativeInit();

        //Log.v("SDL", "SDL thread terminated");
    }
}


/**
    SDLSurface. This is what we draw on, so we need to know when it's created
    in order to do anything useful. 

    Because of this, that's where we set up the SDL thread
*/
class SDLSurface extends SurfaceView implements SurfaceHolder.Callback, 
    View.OnKeyListener, View.OnTouchListener, SensorEventListener  {

    // Sensors
    private static SensorManager mSensorManager;

    // Keep track of the surface size to normalize touch events
    private static float mWidth, mHeight;

    // Startup    
    public SDLSurface(Context context) {
        super(context);
        getHolder().addCallback(this); 
    
        setFocusable(true);
        setFocusableInTouchMode(true);
        requestFocus();
        setOnKeyListener(this); 
        setOnTouchListener(this);
        setKeepScreenOn(true);

		setWillNotDraw(true);

        for(int i=0; i<maxp; i++)
            pid[i] = -1;

        mSensorManager = (SensorManager)context.getSystemService("sensor");

        // Some arbitrary defaults to avoid a potential division by zero
        mWidth = 1.0f;
        mHeight = 1.0f;
    }

    // Called when we have a valid drawing surface
    public void surfaceCreated(SurfaceHolder holder) {
        Log.v("SDL", "surfaceCreated()");
        holder.setType(SurfaceHolder.SURFACE_TYPE_GPU);
        enableSensor(Sensor.TYPE_ACCELEROMETER, false);
    }

    // Called when we lose the surface
    public void surfaceDestroyed(SurfaceHolder holder) {
        Log.v("SDL", "surfaceDestroyed()");
        if (!SDLActivity.mIsPaused) {
            SDLActivity.mIsPaused = true;
            SDLActivity.nativePause();
        }
        enableSensor(Sensor.TYPE_ACCELEROMETER, false);
    }

    // Called when the surface is resized
    public void surfaceChanged(SurfaceHolder holder,
                               int format, int width, int height) {
        Log.v("SDL", "surfaceChanged()");

        int sdlFormat = 0x85151002; // SDL_PIXELFORMAT_RGB565 by default
        switch (format) {
        case PixelFormat.A_8:
            Log.v("SDL", "pixel format A_8");
            break;
        case PixelFormat.LA_88:
            Log.v("SDL", "pixel format LA_88");
            break;
        case PixelFormat.L_8:
            Log.v("SDL", "pixel format L_8");
            break;
        case PixelFormat.RGBA_4444:
            Log.v("SDL", "pixel format RGBA_4444");
            sdlFormat = 0x85421002; // SDL_PIXELFORMAT_RGBA4444
            break;
        case PixelFormat.RGBA_5551:
            Log.v("SDL", "pixel format RGBA_5551");
            sdlFormat = 0x85441002; // SDL_PIXELFORMAT_RGBA5551
            break;
        case PixelFormat.RGBA_8888:
            Log.v("SDL", "pixel format RGBA_8888");
            sdlFormat = 0x86462004; // SDL_PIXELFORMAT_RGBA8888
            break;
        case PixelFormat.RGBX_8888:
            Log.v("SDL", "pixel format RGBX_8888");
            sdlFormat = 0x86262004; // SDL_PIXELFORMAT_RGBX8888
            break;
        case PixelFormat.RGB_332:
            Log.v("SDL", "pixel format RGB_332");
            sdlFormat = 0x84110801; // SDL_PIXELFORMAT_RGB332
            break;
        case PixelFormat.RGB_565:
            Log.v("SDL", "pixel format RGB_565");
            sdlFormat = 0x85151002; // SDL_PIXELFORMAT_RGB565
            break;
        case PixelFormat.RGB_888:
            Log.v("SDL", "pixel format RGB_888");
            // Not sure this is right, maybe SDL_PIXELFORMAT_RGB24 instead?
            sdlFormat = 0x86161804; // SDL_PIXELFORMAT_RGB888
            break;
        default:
            Log.v("SDL", "pixel format unknown " + format);
            break;
        }

        mWidth = (float) width;
        mHeight = (float) height;
        SDLActivity.onNativeResize(width, height, sdlFormat);
        Log.v("SDL", "Window size:" + width + "x"+height);

        SDLActivity.startApp();
    }

    // unused
    public void onDraw(Canvas canvas) {}


    // Key events
    public boolean onKey(View  v, int keyCode, KeyEvent event) {

        if (keyCode==KeyEvent.KEYCODE_VOLUME_DOWN || keyCode==KeyEvent.KEYCODE_VOLUME_UP)
        {
            return false;
        }
        
        //remap keys, for sdl doesn't support them
        switch(keyCode)
        {
            case KeyEvent.KEYCODE_BUTTON_1://	Key code constant: Generic Game Pad Button #1.
                keyCode=KeyEvent.KEYCODE_1; break;                
            case KeyEvent.KEYCODE_BUTTON_10: //key code constant: Generic Game Pad Button #10.
                keyCode=KeyEvent.KEYCODE_2; break;
            case KeyEvent.KEYCODE_BUTTON_11: //key code constant: Generic Game Pad Button #11.
                keyCode=KeyEvent.KEYCODE_3; break;
            case KeyEvent.KEYCODE_BUTTON_12: //key code constant: Generic Game Pad Button #12.
                keyCode=KeyEvent.KEYCODE_4; break;
            case KeyEvent.KEYCODE_BUTTON_13: //key code constant: Generic Game Pad Button #13.
                keyCode=KeyEvent.KEYCODE_5; break;
            case KeyEvent.KEYCODE_BUTTON_14: //key code constant: Generic Game Pad Button #14.
                keyCode=KeyEvent.KEYCODE_6; break;
            case KeyEvent.KEYCODE_BUTTON_15: //key code constant: Generic Game Pad Button #15.
                keyCode=KeyEvent.KEYCODE_7; break;
            case KeyEvent.KEYCODE_BUTTON_16: //key code constant: Generic Game Pad Button #16.
                keyCode=KeyEvent.KEYCODE_8; break;
            case KeyEvent.KEYCODE_BUTTON_2: //key code constant: Generic Game Pad Button #2.
                keyCode=KeyEvent.KEYCODE_9; break;
            case KeyEvent.KEYCODE_BUTTON_3: //key code constant: Generic Game Pad Button #3.
                keyCode=KeyEvent.KEYCODE_0; break;
            case KeyEvent.KEYCODE_BUTTON_4: //key code constant: Generic Game Pad Button #4.
                keyCode=KeyEvent.KEYCODE_A; break;
            case KeyEvent.KEYCODE_BUTTON_5: //key code constant: Generic Game Pad Button #5.
                keyCode=KeyEvent.KEYCODE_B; break;
            case KeyEvent.KEYCODE_BUTTON_6: //key code constant: Generic Game Pad Button #6.
                keyCode=KeyEvent.KEYCODE_C; break;
            case KeyEvent.KEYCODE_BUTTON_7: //key code constant: Generic Game Pad Button #7.
                keyCode=KeyEvent.KEYCODE_D; break;
            case KeyEvent.KEYCODE_BUTTON_8: //key code constant: Generic Game Pad Button #8.
                keyCode=KeyEvent.KEYCODE_E; break;
            case KeyEvent.KEYCODE_BUTTON_9: //key code constant: Generic Game Pad Button #9.
                keyCode=KeyEvent.KEYCODE_F; break;
            case KeyEvent.KEYCODE_BUTTON_A: //key code constant: A Button key.
                keyCode=KeyEvent.KEYCODE_G; break;
            case KeyEvent.KEYCODE_BUTTON_B: //key code constant: B Button key.
                keyCode=KeyEvent.KEYCODE_H; break;
            case KeyEvent.KEYCODE_BUTTON_C: //key code constant: C Button key.
                keyCode=KeyEvent.KEYCODE_I; break;
            case KeyEvent.KEYCODE_BUTTON_L1: //key code constant: L1 Button key.
                keyCode=KeyEvent.KEYCODE_J; break;
            case KeyEvent.KEYCODE_BUTTON_L2: //key code constant: L2 Button key.
                keyCode=KeyEvent.KEYCODE_K; break;
            case KeyEvent.KEYCODE_BUTTON_MODE: //key code constant: Mode Button key.
                keyCode=KeyEvent.KEYCODE_L; break;
            case KeyEvent.KEYCODE_BUTTON_R1: //key code constant: R1 Button key.
                keyCode=KeyEvent.KEYCODE_M; break;
            case KeyEvent.KEYCODE_BUTTON_R2: //key code constant: R2 Button key.
                keyCode=KeyEvent.KEYCODE_N; break;
            case KeyEvent.KEYCODE_BUTTON_SELECT: //key code constant: Select Button key.
                keyCode=KeyEvent.KEYCODE_O; break;
            case KeyEvent.KEYCODE_BUTTON_START: //key code constant: Start Button key.
                keyCode=KeyEvent.KEYCODE_P; break;
            case KeyEvent.KEYCODE_BUTTON_THUMBL: //key code constant: Left Thumb Button key.
                keyCode=KeyEvent.KEYCODE_Q; break;
            case KeyEvent.KEYCODE_BUTTON_THUMBR: //key code constant: Right Thumb Button key.
                keyCode=KeyEvent.KEYCODE_R; break;
            case KeyEvent.KEYCODE_BUTTON_X: //key code constant: X Button key.
                keyCode=KeyEvent.KEYCODE_S; break;
            case KeyEvent.KEYCODE_BUTTON_Y: //key code constant: Y Button key.
                keyCode=KeyEvent.KEYCODE_T; break;
            case KeyEvent.KEYCODE_BUTTON_Z: //key code constant: Z Button key.
                keyCode=KeyEvent.KEYCODE_U; break;
        }

        if (event.getAction() == KeyEvent.ACTION_DOWN) {
            //Log.v("SDL", "key down: " + keyCode);
            SDLActivity.onNativeKeyDown(keyCode);
            return true;
        }
        else if (event.getAction() == KeyEvent.ACTION_UP) {
            //Log.v("SDL", "key up: " + keyCode);
            SDLActivity.onNativeKeyUp(keyCode);
            return true;
        }
        
        return false;
    }

    //pointer states
    private final int maxp = 30;
    private float px[] = new float[maxp];
    private float py[] = new float[maxp];
    private int pid[] = new int[maxp];

    // Touch events
    public boolean onTouch(View v, MotionEvent event) {
        
        {
             final int touchDevId = event.getDeviceId();
             final int pointerCount = event.getPointerCount();
             // touchId, pointerId, action, x, y, pressure
             int actionPointerIndex = event.getActionIndex(), pointerFingerId;
             int action = event.getActionMasked();

             float x,y,p,s;

             if(action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_CANCEL)
             {
                for(int i=0; i<maxp; i++)
                {
                    pid[i] = -1;
                }
             }
             else
             {
                 for (int i = 0; i < pointerCount; i++)
                 {
                     pointerFingerId = event.getPointerId(i);
                     x = event.getX(i);
                     y = event.getY(i);
                     if(i!=actionPointerIndex || action == MotionEvent.ACTION_MOVE)
                     {
                         for(int j=0; j<maxp; j++)//handle move
                         {
                            if(pid[j]==pointerFingerId) 
                            {
                                px[j] = x;
                                py[j] = y;
                                break;
                            }
                         }
                     }
                     else if(action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_POINTER_DOWN)
                     {
                         for(int j=0; j<maxp; j++)//handle down
                         {
                            if(pid[j]<0) 
                            {
                                pid[j] = pointerFingerId;
                                px[j] = x;
                                py[j] = y;
                                break;
                            }
                         }
                     }
                     else if(action == MotionEvent.ACTION_POINTER_UP)
                     {
                         for(int j=0; j<maxp; j++)//handle up
                         {
                            if(pid[j]==pointerFingerId) 
                            {
                                pid[j] = -1;
                                break;
                            }
                         }
                     }
                 }
             }

            SDLActivity.nativeUpdateTouchStates(px, py, pid, maxp);

             if (action == MotionEvent.ACTION_MOVE && pointerCount > 1) {
                // TODO send motion to every pointer if its position has
                // changed since prev event.
                for (int i = 0; i < pointerCount; i++) {
                    pointerFingerId = event.getPointerId(i);
                    x = event.getX(i);
                    y = event.getY(i);
                    p = event.getPressure(i);
                    SDLActivity.onNativeTouch(touchDevId, pointerFingerId, action, x, y, p);
                }
             } else {
                pointerFingerId = event.getPointerId(actionPointerIndex);
                x = event.getX(actionPointerIndex);
                y = event.getY(actionPointerIndex);
                p = event.getPressure(actionPointerIndex);
                SDLActivity.onNativeTouch(touchDevId, pointerFingerId, action, x, y, p);
             }
             
        }

      return true;
   }

    // Sensor events
    public void enableSensor(int sensortype, boolean enabled) {
        // TODO: This uses getDefaultSensor - what if we have >1 accels?
        if (enabled) {
            mSensorManager.registerListener(this, 
                            mSensorManager.getDefaultSensor(sensortype), 
                            SensorManager.SENSOR_DELAY_GAME, null);
        } else {
            mSensorManager.unregisterListener(this, 
                            mSensorManager.getDefaultSensor(sensortype));
        }
    }
    
    public void onAccuracyChanged(Sensor sensor, int accuracy) {
        // TODO
    }

    public void onSensorChanged(SensorEvent event) {
        if (event.sensor.getType() == Sensor.TYPE_ACCELEROMETER) {
            SDLActivity.onNativeAccel(event.values[0] / SensorManager.GRAVITY_EARTH,
                                      event.values[1] / SensorManager.GRAVITY_EARTH,
                                      event.values[2] / SensorManager.GRAVITY_EARTH);
        }
    }
    
}

/* This is a fake invisible editor view that receives the input and defines the
 * pan&scan region
 */
class DummyEdit extends View implements View.OnKeyListener {
    InputConnection ic;

    public DummyEdit(Context context) {
        super(context);
        setFocusableInTouchMode(true);
        setFocusable(true);
        setOnKeyListener(this);
    }

    @Override
    public boolean onCheckIsTextEditor() {
        return true;
    }

    public boolean onKey(View v, int keyCode, KeyEvent event) {

        // This handles the hardware keyboard input
        if (event.isPrintingKey()) {
            if (event.getAction() == KeyEvent.ACTION_DOWN) {
                ic.commitText(String.valueOf((char) event.getUnicodeChar()), 1);
            }
            return true;
        }

        if (event.getAction() == KeyEvent.ACTION_DOWN) {
            SDLActivity.onNativeKeyDown(keyCode);
            return true;
        } else if (event.getAction() == KeyEvent.ACTION_UP) {
            SDLActivity.onNativeKeyUp(keyCode);
            return true;
        }

        return false;
    }

    @Override
    public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
        ic = new SDLInputConnection(this, true);

        outAttrs.imeOptions = EditorInfo.IME_FLAG_NO_EXTRACT_UI
                | 33554432 /* API 11: EditorInfo.IME_FLAG_NO_FULLSCREEN */;

        return ic;
    }
}

class SDLInputConnection extends BaseInputConnection {

    public SDLInputConnection(View targetView, boolean fullEditor) {
        super(targetView, fullEditor);

    }

    @Override
    public boolean sendKeyEvent(KeyEvent event) {

        /*
         * This handles the keycodes from soft keyboard (and IME-translated
         * input from hardkeyboard)
         */
        int keyCode = event.getKeyCode();
        if (event.getAction() == KeyEvent.ACTION_DOWN) {

            SDLActivity.onNativeKeyDown(keyCode);
            return true;
        } else if (event.getAction() == KeyEvent.ACTION_UP) {

            SDLActivity.onNativeKeyUp(keyCode);
            return true;
        }
        return super.sendKeyEvent(event);
    }

    @Override
    public boolean commitText(CharSequence text, int newCursorPosition) {

        nativeCommitText(text.toString(), newCursorPosition);

        return super.commitText(text, newCursorPosition);
    }

    @Override
    public boolean setComposingText(CharSequence text, int newCursorPosition) {

        nativeSetComposingText(text.toString(), newCursorPosition);

        return super.setComposingText(text, newCursorPosition);
    }

    public native void nativeCommitText(String text, int newCursorPosition);

    public native void nativeSetComposingText(String text, int newCursorPosition);

}
