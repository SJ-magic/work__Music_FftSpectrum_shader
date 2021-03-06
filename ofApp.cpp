#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	/********************
		ofSetVerticalSync(true); 
	の時、処理がどんなに速く終わっても、Displayの垂直同期を待ってからの描画になるので、60fpsが頭打ち。
	
	Speed checkしたい時は、
		ofSetFrameRate(0);
		ofSetVerticalSync(false); 
		
	実動作時は、ブンブンに回る結果ファンがうるさくなるので、
		ofSetFrameRate(60);
		ofSetVerticalSync(true); 
		
	あたりが良さそう。
	********************/
	/*
	ofSetFrameRate(0);
	ofSetVerticalSync(false);
	/*/
	ofSetFrameRate(60);
	ofSetVerticalSync(true);
	// */
	
	/********************
	********************/
	b_fullScreen = false;
	b_disp_Time_and_FrameRate = false;
	
	ofEnableSmoothing();

	// ofEnableDepthTest(); // 注意!!!! 2d imageで、αが効かなくなる.
	
	ofSetWindowShape(WIDTH, HEIGHT);
	
	/********************
	********************/
	SpectrumIndicator.setup(N);
	img_Jacket.loadImage( "jacket.png" );
	
	/********************
	********************/
	video.loadMovie("GoodVibesTimeLapse Video.mov");
	video.setLoopState(OF_LOOP_NORMAL);
	video.play();
	video.setSpeed(1);
	video.setVolume(0);

	
	/********************
	********************/
	b_Particle = false;
	ParticleSet.setup();
	
	/********************
	********************/
	// sound.load( "ClubLife by Tiesto Podcast 465.wav" );	
	// sound.load( "ClubLife by Tiesto Podcast 471.wav" );
	sound.load( "ClubLife by Tiesto Podcast 485.wav" );
	// sound.load( "faithless.wav" );	
	
	sound.setLoop( true );
	sound.play();
	
	// ここでvolume cnotrolすると、fft結果に影響が出るので注意
	// 全体音量でControlする分には、影響が出ない.
	// sound.setVolume(0.0);

	//Set spectrum values to 0
	for (int i=0; i<N; i++) {
		spectrum[i] = 0.0f;
	}
	
	/********************
	********************/
	State = STATE_NONE;

	/********************
	********************/
	gui.setup();
	
	gui.add(gui_param[0].setup("smooth", 0.05, 0.01, 0.2));
	gui.add(gui_param[1].setup("NonLinear", 0.02, 0.01, 0.5));
	
	/********************
	********************/
	b_DispGui = false;
	b_showCursor = true;
}

//--------------------------------------------------------------
void ofApp::update(){
	/********************
	********************/
	// sound.setVolume(volume);
	ofSoundUpdate();	

	//Get current spectrum with N bands
	float *val = ofSoundGetSpectrum( N );
	//We should not release memory of val,
	//because it is managed by sound engine

	/********************
	********************/
	float val_ave[N];
	
	/* */
	static float LastINT_sec = 0;
	float now = ofGetElapsedTimef();
	float dt = ofClamp(now - LastINT_sec, 0, 0.1);
	LastINT_sec = now;
	
	/* */
	// const double SmoothFilterThreshTime = 0.05; // 小さくする程Responceが良くなる.
	const double SmoothFilterThreshTime = 0.03;
	double tangent = 1 / SmoothFilterThreshTime;
	
	double SmoothFilterAlpha;
	if(dt < SmoothFilterThreshTime)	SmoothFilterAlpha = tangent * dt;
	else							SmoothFilterAlpha = 1;
	
	/* */
	// const double NonLinearFilter_ThreshLev = 0.02;
	const double NonLinearFilter_ThreshLev = 0.08;
	const double NonLinearFilter_k = 1/NonLinearFilter_ThreshLev;
	
	/* */
	const float Down_per_ms = 0.05 / 16.6;
	float DownRatio = Down_per_ms * dt * 1000;
	
	/* */
	for ( int i=0; i<N; i++ ) {
		/* */
		val_ave[i] = SmoothFilterAlpha * val[i] + (1 - SmoothFilterAlpha) * spectrum[i];
		
		/* */
		double diff = val_ave[i] - spectrum[i];
		if( (0 <= diff) && (diff < NonLinearFilter_ThreshLev) ){
			diff = NonLinearFilter_k * pow(diff, 2);
		}else if( (-NonLinearFilter_ThreshLev < diff) && (diff < 0) ){
			diff = -NonLinearFilter_k * pow(diff, 2);
		}else{
			diff = diff;
		}
		float val_NonLinearFilter_out = spectrum[i] + diff;
		
		/* */
		spectrum[i] *= (1 - DownRatio);
		spectrum[i] = max( spectrum[i], val_NonLinearFilter_out );
	}
	

	/********************
	********************/
	if(BootMode == BOOT_MODE__COLOR_CHANGE){
		int ColorThemeId = ColorThemeTable.IsColorThemeChange( sound.getPositionMS() );
		if(ColorThemeId != -1){
			switch(ColorThemeId){
				case 0:
					printf("\nAqua\n");
					break;
				case 1:
					printf("\nMagma\n");
					break;
				case 2:
					printf("\nDigital\n");
					break;
				case 3:
					printf("\nsexy\n");
					break;
				case 4:
					printf("\nTrip\n");
					break;
				case 5:
					printf("\nReggae\n");
					break;
				case 6:
					printf("\nSamba\n");
					break;
				case 7:
					printf("\nSweets\n");
					break;
			}
			
			/********************
			********************/
			SpectrumIndicator.load_ColorTheme_setting(ColorThemeId);
			ParticleSet.load_ColorTheme_setting(ColorThemeId);
			
			/********************
			********************/
			if(TimingAdjust_SpectrumIndicator.get__b_Adjust()){
				SpectrumIndicator.change_IndicatorType(TimingAdjust_SpectrumIndicator.get__NextValue());
			}
			if(TimingAdjust_SpectrumIndicator_point.get__b_Adjust()){
				TimingAdjust_SpectrumIndicator_point.get__NextValue(); // clear.
				
				SpectrumIndicator.toggle_PointIndicator();
			}
			if(TimingAdjust_Particle.get__b_Adjust()){
				TimingAdjust_Particle.get__NextValue(); // clear.
				
				b_Particle = !b_Particle;
				printf("\nparticle = %d\n", b_Particle);
			}

		}
	}
	
	/********************
	********************/
	SpectrumIndicator.update();
	
	/* 無効時もupdateし、減速させておく */
	// if(b_Particle) ParticleSet.update(mouseX, mouseY);
	ParticleSet.update(mouseX, mouseY);
	
	/********************
	********************/
	video.update();
}

//--------------------------------------------------------------
void ofApp::draw(){
	/********************
	********************/
	ofBackground(0, 0, 0, 0);
	ofSetColor(255, 255, 255, 255);
	
	/********************
	********************/
	// img_Jacket.draw(0, 0, ofGetWidth(), ofGetHeight());
	video.draw( 0, 0, ofGetWidth(), ofGetHeight());
	
	/********************
	********************/
	ofEnableAlphaBlending();
	ofEnableBlendMode(OF_BLENDMODE_ADD);
	// ofEnableBlendMode(OF_BLENDMODE_ALPHA);
	
	SpectrumIndicator.draw(spectrum);
	

	/********************
	********************/
	if(b_Particle)	ParticleSet.draw();
	
	/********************
	********************/
	ofSetColor(255, 255, 255, 255);
	
	if(b_disp_Time_and_FrameRate){
		print_musicTime();
	}
	
	print_mousePosition();
	
	// gui.draw();
}

/******************************
******************************/
void ofApp::print_mousePosition()
{
	/********************
	********************/
	int pos_ms = sound.getPositionMS();
	
	int min		= pos_ms / 1000 / 60;
	int sec		= pos_ms / 1000 - min * 60;
	int ms		= pos_ms % 1000;
	int frame	= ms * 30 / 1000;
	
	/********************
	********************/
	float x = float(mouseX) / ofGetWidth();
	float y = float(mouseY) / ofGetHeight();
	
	printf("(%5.2f, %5.2f)\t%6d:%6d:%6d\r", x, y, min, sec, ms);
	fflush(stdout);
}

/******************************
******************************/
void ofApp::print_musicTime()
{
	int pos_ms = sound.getPositionMS();
	
	int min		= pos_ms / 1000 / 60;
	int sec		= pos_ms / 1000 - min * 60;
	int ms		= pos_ms % 1000;
	int frame	= ms * 30 / 1000;
	
	/*
	printf("%6d:%6d:%6d\r", min, sec, ms);
	fflush(stdout);
	*/
	
	/********************
	ofToString
		http://openframeworks.cc/documentation/utils/ofUtils/#show_ofToString
	********************/
	{
		string info;
		// info += ofToString(min, 4, ' ') + ":" + ofToString(sec, 4, ' ') + ":" + ofToString(ms, 4, ' ');
		info += ofToString(min, 4, ' ') + ":" + ofToString(sec, 4, ' ') + ":" + ofToString(frame, 4, ' ');
		ofDrawBitmapString(info, 30, 30);
	}
	{
		string info;
		info += "FPS = " + ofToString(ofGetFrameRate(), 2);
		ofDrawBitmapString(info, 30, 50);
	}
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	switch(key){
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			if(State == STATE_NONE){
				int type = key - '0';
				if(type < SpectrumIndicator.get_NumIndicatorTypes()){
					if(BootMode != BOOT_MODE__COLOR_CHANGE){
						SpectrumIndicator.change_IndicatorType(type);
					}else{
						if(ColorThemeTable.IsColorThemeChange_soon(sound.getPositionMS())){
							TimingAdjust_SpectrumIndicator.set__NextValue(type);
						}else{
							SpectrumIndicator.change_IndicatorType(type);
						}
					}
				}
				
			}else if(State == STATE_SEEK_INPUT){
				if(SeekInput_id < INPUT_SIZE){
					SeekInput[SeekInput_id] = key - '0';
					SeekInput_id++;
				}
				
			}else if(State == STATE_LOAD_GUI_SETTING){
				int type = key - '0';
				if(type < 3) SpectrumIndicator.load_setting(type);
				
				State = STATE_NONE;
			}
			
			break;
			
		case 'a':
			ofSetWindowShape(WIDTH, HEIGHT);
			break;
			
		case 'b':
			ofSetWindowShape(WIDTH_SMALL, HEIGHT_SMALL);
			break;
			
		case 'c':
		{
			ofImage ScreenImage;
			ScreenImage.grabScreen(0, 0, ofGetWidth(), ofGetHeight());
			ScreenImage.saveImage("screen.png");
		}
			break;
			
		case 'd':
			b_disp_Time_and_FrameRate = !b_disp_Time_and_FrameRate;
			break;
			
		case 'f':
			b_fullScreen = !b_fullScreen;
			ofSetFullscreen(b_fullScreen);
			
			if(!b_fullScreen)	ofSetWindowShape(WIDTH, HEIGHT);

			break;
			
		case 'g':
			b_DispGui = !b_DispGui;
			SpectrumIndicator.set_dispGui(b_DispGui);
			ParticleSet.set_dispGui(b_DispGui);
			break;
			
		case 'h':
			ofHideCursor();
			b_showCursor = false;
			break;
			
		case 'i':
			ofShowCursor();
			b_showCursor = true;
			break;
			
		case 'k':
			switch(State){
				case STATE_NONE:
					State = STATE_SEEK_INPUT;
					SeekInput_id = 0;
					
					printf("\nSeek To[sec]:\n");
					break;
					
				case STATE_SEEK_INPUT:
					State = STATE_NONE;
					printf("escape\n");
					break;
			}
			break;
			
		case 'm':
			printf("\n");
			printf("a\tWindow Size = Large\n");
			printf("b\tWindow Size = Small\n");
			printf("c\tScreen capture\n");
			printf("d\tdisp Frame rate\n");
			printf("f\tfull screen\n");
			printf("g\tgui\n");
			printf("k\tseek\n");
			printf("s\tsave gui setting\n");
			printf("x\tLoad gui setting\n");
			break;
			
		case 'p':
			if(BootMode != BOOT_MODE__COLOR_CHANGE){
				b_Particle = !b_Particle;
				printf("\nparticle = %d\n", b_Particle);
			}else{
				if(ColorThemeTable.IsColorThemeChange_soon(sound.getPositionMS())){
					TimingAdjust_Particle.set__NextValue(1);
				}else{
					b_Particle = !b_Particle;
					printf("\nparticle = %d\n", b_Particle);
				}
			}
			break;
			
		case 's':
			SpectrumIndicator.save_setting();
			ParticleSet.save_setting();
			break;
		
		case 'x':
			printf("\nLoad gui setting\n");
			switch(State){
				case STATE_NONE:
					State = STATE_LOAD_GUI_SETTING;
					printf("\t0:Straight\n");
					printf("\t1:Circle\n");
					printf("\t2:Point\n");
					break;
					
				case STATE_LOAD_GUI_SETTING:
					State = STATE_NONE;
					printf("escape\n");
					break;
			}
			break;
			
		case OF_KEY_RETURN:
			if(State == STATE_SEEK_INPUT){
				SeekInput_id--;
				if(SeekInput_id < 0){
					// nothing.
					
				}else{
					// try seek.
					int i = 0;
					int input_val_sec = 0;
					
					while(0 <= SeekInput_id){
						input_val_sec += SeekInput[i] * pow(10, SeekInput_id);
						SeekInput_id--;
						i++;
					}
					
					printf("%d\n", input_val_sec);
					
					if(IsSeekTarget_inRange(input_val_sec)){
						sound.setPositionMS(input_val_sec * 1000);
						ColorThemeTable.reset();
						
						video.setPosition( input_val_sec / video.getDuration() );
					}
				}
			}
			State = STATE_NONE;
			break;
			
		case ' ':
			if(BootMode != BOOT_MODE__COLOR_CHANGE){
				SpectrumIndicator.toggle_PointIndicator();
			}else{
				if(ColorThemeTable.IsColorThemeChange_soon(sound.getPositionMS())){
					TimingAdjust_SpectrumIndicator_point.set__NextValue(1);
				}else{
					SpectrumIndicator.toggle_PointIndicator();
				}
			}
			
			break;
	}
}

/******************************
******************************/
bool ofApp::IsSeekTarget_inRange(int sec)
{
	/********************
	経過時間が短すぎるので、精度出ないかもしれないので、userにretryしてもらう.
	********************/
	if(sound.getPositionMS() < 1000){
		printf("please try again\n");
		return false;
	}
	
	/********************
	********************/
	if(sec * 1000 < (sound.getPositionMS() / sound.getPosition()) - 5000 ){
		return true;
	}else{
		printf("out of range\n");
		return false;
	}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
	// b_Particle 無効時もonにしておき、裏で操作可能にしておく
	ParticleSet.set_attractive(true);
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
	// b_Particle 無効時もonにしておき、裏で操作可能にしておく
	ParticleSet.set_attractive(false);
}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
