#include "gui.h"

#include <time.h>
#include <iostream>
#include <fstream>

#include "FileLoader.h"
#include "JumpFlooding.h"

GLuint g_time_query[10];
std::ofstream g_fout;
float g_viewScale = 1.0f;
FILE *g_fp;

void PrintMSTimes(std::string str, std::string& dst, GLuint64 ms, clock_t &total_ms) {
	total_ms += (ms / 1e6);
	dst += str + ", " + std::to_string((ms / 1e6)) + ", ";
}

void renderAll(const GLuint *tex) {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 1, 0, 1, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glViewport(0, 0, g_screenSize.cx, g_screenSize.cy);
	glEnable(GL_TEXTURE_2D);

	static GLuint quad = 0;

	if (!quad) {
		quad = glGenLists(1);
		glNewList(quad, GL_COMPILE);
		glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f);	glVertex2f(0.0f, 0.0f);
		glTexCoord2f(1.0f, 0.0f);	glVertex2f(1.0f, 0.0f);
		glTexCoord2f(1.0f, 1.0f);   glVertex2f(1.0f, 1.0f);
		glTexCoord2f(0.0f, 1.0f);   glVertex2f(0.0f, 1.0f);
		glEnd();
		glEndList();
	}
	// 입력
	if (g_tex != 0) {
		glBindTexture(GL_TEXTURE_2D, *tex);
		glPushMatrix();
		glCallList(quad);
		glPopMatrix();
	}
}

int main(int argc, char** argv)
{
	/* 창 초기화, 에러 핸들링 등록, 이벤트 콜백 등록, OpenGL 초기화 */
	/* -------------------------------------------------------------------------------------- */


	glfwInit();
	glfwSetErrorCallback([](int err, const char* desc) { puts(desc); });
	
	if (argc > 1) {
		g_screenSize.cx = atoi(argv[1]);
		g_screenSize.cy = atoi(argv[2]);
		g_obj_name = std::string(argv[3]);
		framebufferSizeCallback(nullptr, g_screenSize.cx, g_screenSize.cy);
	}
	else {
		framebufferSizeCallback(nullptr, 1024, 1024);
	}



	glfwWindowHint(GLFW_DECORATED, 0);
	GLFWwindow *window = glfwCreateWindow(g_screenSize.cx, g_screenSize.cy, "3D Voronoi NNI Rendering", nullptr, nullptr);

	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
	glfwSetCursorPosCallback(window, cursorPosCallback);
	glfwSetKeyCallback(window, keyboardCallback);


	glewInit();

	/* 객체 생성 */
	/* -------------------------------------------------------------------------------------- */
	JumpFlooding * g_JFRenderer = new JumpFlooding();

	Shader* g_normalShader = new Shader("./shader/nVS.glsl", "./shader/nFS.glsl");

	/* 초기화 */
	RawFormat volumeFormat;
	byte* volumeData = loadRawFile("../data/bucky.bin", volumeFormat.w, volumeFormat.h, volumeFormat.d, volumeFormat.c);

	// 3차원 텍스쳐 생성
	GLuint id;
	glGenTextures(1, &id);
	glEnable(GL_TEXTURE_3D);
	glBindTexture(GL_TEXTURE_3D, id);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB32F, volumeFormat.w, volumeFormat.h, volumeFormat.d, 0, GL_BGR,
		GL_UNSIGNED_BYTE, volumeData);
	glDisable(GL_TEXTURE_3D);

	// 카메라 위치 버퍼
	GLuint cameraUBO;
	glGenBuffers(1, &cameraUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, cameraUBO);

	glBufferData(GL_UNIFORM_BUFFER, 144, nullptr, GL_STATIC_DRAW);

	glm::vec3 pos = g_camera.getPosition();
	glBufferSubData(GL_UNIFORM_BUFFER, 0, 12, &pos[0]);
	glBufferSubData(GL_UNIFORM_BUFFER, 16, 64, g_camera.pmat());
	glBufferSubData(GL_UNIFORM_BUFFER, 80, 64, g_camera.vmat());
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	// 1에 연결
	glBindBufferBase(GL_UNIFORM_BUFFER, 2, cameraUBO);


	/* 성능측정 */
	g_fout.open("../Report/opti_performance_report.csv");
	/* -------------------------------------------------------------------------------------- */

	g_camera.setRotation(glm::quat());
	g_camera.setPosition(glm::vec3(0, 0, 5));


	g_camera.lookAt(glm::vec3(0.0f));
	g_camera.setProjectMode(Camera::PM_Perspective, 45, 0.1f, 500.1f);
	g_camera.setScreen(glm::vec2(g_screenSize.cx, g_screenSize.cy));
	g_camera.setViewport(glm::vec4(0, 0, g_screenSize.cx, g_screenSize.cy));

	// 성능측정 리포트
	char report[128] = { 0 };
	sprintf_s(report, "../Report/report.csv");
	fopen_s(&g_fp, report, "w");


	/* 메인 루프 */
	/* -------------------------------------------------------------------------------------- */
	int save_count = 0;
	glGenQueries(sizeof(g_time_query) / sizeof(GLuint), g_time_query);
	while (!glfwWindowShouldClose(window))
	{
		GLuint64 elapsed_time = 0.0f; int done = 0;
		clock_t total_t = 0;
		std::string text = "";
		int gl_query_idx = 0;

		// 이벤트 핸들링
		/* -------------------------------------------------------------------------------------- */
		glfwPollEvents();


		// GO TO
		/* -------------------------------------------------------------------------------------- */
		if (g_shader_init) {
			delete g_normalShader;
			g_normalShader = new Shader("./shader/nVS.glsl", "./shader/nFS.glsl");

			g_JFRenderer->resetShader();
			g_shader_init = false;
		}

		// 성능측정
		//static int count_k = 0; static float trace_speed_acc = 0.0f;
		//if (my_optix->m_accumFrame > 5) {
		//	if (g_apectureSize >= 0.0f) {
		//		if (count_k != 0 && count_k % 10 == 0) {
		//			g_fout << "number of ray, " << trace_sample << ", " << trace_speed_acc / 10.0f << std::endl;
		//			//printf("aspecture: %f, rendering time: %f\n", g_apectureSize, trace_speed_acc);
		//			g_apectureSize -= 0.01f;
		//			trace_speed_acc = 0.0f;
		//		}
		//		else {
		//			trace_speed_acc += elapsed_time;
		//		}
		//	}
		//	else {
		//		g_fout.close();
		//		exit(0);
		//	}
		//	count_k++;
		//}

		GLuint src = 0;

		// Jump Flooding
		g_JFRenderer->render(src, &g_time_query[gl_query_idx], &elapsed_time, &done);
		gl_query_idx++;
		GLuint jt = g_JFRenderer->colorTex;
		GLuint coord = g_JFRenderer->coordTex;
		PrintMSTimes("JPA", text, elapsed_time, total_t);

		total_t += (elapsed_time / 1e6);
		g_FPS = 1.0f / round(1E3 / total_t);

		text += "display," + std::to_string(elapsed_time / 1e6) + ", ";
		text += "Total," + std::to_string(total_t) + ", ";
		text += "FPS," + std::to_string(round(1E3 / total_t)) + ", ";
		text += "apercture, " + std::to_string(g_apectureSize) + ", " + "g_number_of_ray, " + std::to_string(g_number_of_ray) + "\n";
		std::cerr << text;

		// 성능측정
		{
			/*if (my_optix->m_accumFrame > 2 && my_optix->m_accumFrame < 300 + 2)
			{
			fprintf_s(g_fp, "%s", text.c_str());

			}
			if (my_optix->m_accumFrame > 300 + 2) {
			fclose(g_fp);
			exit(0);
			}*/
		}
		// 		text = std::to_string(my_optix->m_accumFrame) + "\n";
		// 		std::cerr << text;

		//if (g_tex != nullptr)
		//	my_optix->render(*g_tex);

		// render normally
		if (g_isTextureChanged > 0) {
			switch (g_isTextureChanged) {
			case 1: g_tex = &jt; break;
			}
			g_isTextureChanged = 0;
		}

		glBeginQuery(GL_TIME_ELAPSED, g_time_query[gl_query_idx]);

		if (g_fullScreen) {
			g_normalShader->use();
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glOrtho(0, 1, 0, 1, -1, 1);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glViewport(0, 0, g_screenSize.cx * g_viewScale, g_screenSize.cy * g_viewScale);

			glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_3D, id);
			glUniform2f(0, (float)g_screenSize.cx * g_viewScale, (float)g_screenSize.cy * g_viewScale);
			glUniform2f(1, (float)g_gaze.x, (float)g_gaze.y);
			glBindBuffer(GL_UNIFORM_BUFFER, cameraUBO);
			glm::vec3 pos = g_camera.getPosition();
			glBufferSubData(GL_UNIFORM_BUFFER, 0, 12, &pos[0]);
			glBufferSubData(GL_UNIFORM_BUFFER, 16, 64, g_camera.pmat());
			glBufferSubData(GL_UNIFORM_BUFFER, 80, 64, g_camera.vmat());
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			glDrawArrays(GL_QUADS, 0, 4);
			glBindTexture(GL_TEXTURE_3D, 0);
			g_normalShader->unuse();
		}
		else {
			renderAll(g_tex);
		}


		glEndQuery(GL_TIME_ELAPSED);
		glGetQueryObjectiv(g_time_query[gl_query_idx], GL_QUERY_RESULT_AVAILABLE, &done);
		glGetQueryObjectui64v(g_time_query[gl_query_idx], GL_QUERY_RESULT, &elapsed_time);


		if (g_saveBMP) {
			char fileName[128] = { 0 };
			sprintf_s(fileName, "%s_%.2f", "bunny_point", g_apectureSize);
			//sprintf_s(fileName, "%s", "bunny");
			saveBMP24(fileName);
			g_saveBMP = false;
		}


		// 스왑 버퍼
		/* -------------------------------------------------------------------------------------- */
		glfwSwapBuffers(window);
		g_camera.setPrevState();
	}
	glDeleteQueries(sizeof(g_time_query) / sizeof(GLuint), g_time_query);

	// 객체 제거
	/* -------------------------------------------------------------------------------------- */
	delete g_JFRenderer;

	// 종료
	/* -------------------------------------------------------------------------------------- */
	glfwTerminate();
}