#include "framework.h"


const char* vertexSource = R"(
	#version 330
    precision highp float;
 
	uniform mat4 MVP;			
 
	layout(location = 0) in vec2 vertexPosition;	
 
	void main() {
		gl_Position = vec4(vertexPosition.x, vertexPosition.y, 0, 1) * MVP; 		
	}
)";


const char* fragmentSource = R"(
	#version 330
    precision highp float;
 
	uniform vec3 color;
	out vec4 fragmentColor;		
 
	void main() {
		fragmentColor = vec4(color, 1); 
	}
)";


struct Camera {
	float wCx, wCy;
	float wWx, wWy;
public:
	Camera() {
		Animate(0);
	}

	mat4 V() {
		return mat4(1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			-wCx, -wCy, 0, 1);
	}

	mat4 P() {
		return mat4(2 / wWx, 0, 0, 0,
			0, 2 / wWy, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1);
	}

	mat4 Vinv() {
		return mat4(1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			wCx, wCy, 0, 1);
	}

	mat4 Pinv() {
		return mat4(wWx / 2, 0, 0, 0,
			0, wWy / 2, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1);
	}

	void Animate(float t) {
		wCx = 0;
		wCy = 0;
		wWx = 20;
		wWy = 20;
	}
};


Camera camera;
bool animate = false;
float tCurrent = 0;
GPUProgram gpuProgram;
int nTesselatedVertices = 1000;
bool IsCRSpline = false;

class Poligon {
	unsigned int vaoPoligon, vboPoligon;
	unsigned int vaoCtrlPoints, vboCtrlPoints;
	vec2			    wTranslate;
protected:
	std::vector<vec4> ControlPoints;
public:
	Poligon() {
		glGenVertexArrays(1, &vaoPoligon);
		glBindVertexArray(vaoPoligon);

		glGenBuffers(1, &vboPoligon);
		glBindBuffer(GL_ARRAY_BUFFER, vboPoligon);

		glEnableVertexAttribArray(0);

		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), NULL);


		glGenVertexArrays(1, &vaoCtrlPoints);
		glBindVertexArray(vaoCtrlPoints);

		glGenBuffers(1, &vboCtrlPoints);
		glBindBuffer(GL_ARRAY_BUFFER, vboCtrlPoints);

		glEnableVertexAttribArray(0);

		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), NULL);

	}

	virtual vec4 r(float t) { return ControlPoints[0]; }
	virtual float tStart() { return 0; }
	virtual float tEnd() { return 1; }

	virtual void AddControlPoint(float cX, float cY) {
		vec4 wVertex = vec4(cX, cY, 0, 1) * camera.Pinv() * camera.Vinv();
		ControlPoints.push_back(wVertex);
	}

	virtual void AddControlPointWithOutTransformation(float cX, float cY) {
		vec4 wVertex = vec4(cX, cY, 0, 1);
		ControlPoints.push_back(wVertex);
	}

	int PickControlPoint(float cX, float cY) {
		vec4 wVertex = vec4(cX, cY, 0, 1) * camera.Pinv() * camera.Vinv();
		for (unsigned int p = 0; p < ControlPoints.size(); p++) {
			if (dot(ControlPoints[p] - wVertex, ControlPoints[p] - wVertex) < 0.1) return p;
		}
		return AddPointWithRightClick(wVertex);
	}

	void MoveControlPoint(int p, float cX, float cY) {
		vec4 wVertex = vec4(cX, cY, 0, 1) * camera.Pinv() * camera.Vinv();
		ControlPoints[p] = wVertex;
	}

	void Draw() {
		mat4 VPTransform = camera.V() * camera.P();

		gpuProgram.setUniform(VPTransform, "MVP");

		int colorLocation = glGetUniformLocation(gpuProgram.getId(), "color");

		if (ControlPoints.size() > 2) {
			std::vector<float> vertexData;
			if (!IsCRSpline) {
				for (unsigned int i = 0; i < ControlPoints.size(); i++) {
					vec4 mVertex = ControlPoints[i];
					vertexData.push_back(mVertex.x);
					vertexData.push_back(mVertex.y);
					vertexData.push_back(1);
					vertexData.push_back(1);
					vertexData.push_back(1);

				}
				glBindBuffer(GL_ARRAY_BUFFER, vboPoligon);
				glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), &vertexData[0], GL_DYNAMIC_DRAW);
				if (colorLocation >= 0) glUniform3f(colorLocation, 1, 1, 1);
				glDrawArrays(GL_LINE_LOOP, 0, vertexData.size() / 5);
			}

			else {
				for (int i = 0; i < nTesselatedVertices; i++) {
					float tNormalized = (float)i / (nTesselatedVertices - 1);
					float t = tStart() + (tEnd() - tStart()) * tNormalized;
					vec4 wVertex = r(t);
					vertexData.push_back(wVertex.x);
					vertexData.push_back(wVertex.y);
				}

				glBindVertexArray(vaoPoligon);
				glBindBuffer(GL_ARRAY_BUFFER, vboPoligon);
				glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), &vertexData[0], GL_DYNAMIC_DRAW);
				if (colorLocation >= 0) glUniform3f(colorLocation, 1, 1, 1);
				glDrawArrays(GL_LINE_LOOP, 0, nTesselatedVertices);
			}
		}

		if (ControlPoints.size() > 0) {
			glBindVertexArray(vaoCtrlPoints);
			glBindBuffer(GL_ARRAY_BUFFER, vboCtrlPoints);
			glBufferData(GL_ARRAY_BUFFER, ControlPoints.size() * 4 * sizeof(float), &ControlPoints[0], GL_DYNAMIC_DRAW);
			if (colorLocation >= 0) glUniform3f(colorLocation, 1, 0, 0);
			glPointSize(10.0f);
			glDrawArrays(GL_POINTS, 0, ControlPoints.size());
		}
	}


	float DistanceBetweenLineAndPoint(vec4 v0, vec4 v1, vec4 v2) {
		return  abs((v2.x - v1.x) * (v1.y - v0.y) - (v1.x - v0.x) * (v2.y - v1.y))
			/ (sqrt(pow(v2.x - v1.x, 2) + pow(v2.y - v1.y, 2)));
	}

	void DeletePoints() {
		int i = 0;
		float minDistance = DistanceBetweenLineAndPoint(ControlPoints[0],
			ControlPoints[ControlPoints.size()],
			ControlPoints[1]);
		float minPosition = 0;
		while (i < round(ControlPoints.size() + 1 / 2)) {
			for (int j = 1; j < ControlPoints.size(); j++) {
				float tmpDistance = DistanceBetweenLineAndPoint(ControlPoints[i],
					ControlPoints[i - 1],
					ControlPoints[i + 1]);
				if (tmpDistance < minDistance)
				{
					minDistance = tmpDistance;
					minPosition = j;
				}
			}
			ControlPoints.erase(std::next(ControlPoints.begin(), minPosition));
			i++;
		}
	}


	float AddPointWithRightClick(vec4 Point) {

		if (IsCRSpline)
			IsCRSpline = false;

		if (ControlPoints.size() <= 1) {
			ControlPoints.push_back(Point);
			return 0;
		}

		else if (ControlPoints.size() == 2) {
			std::vector<vec4> tmp;
			tmp.push_back(ControlPoints.at(0));
			tmp.push_back(Point);
			tmp.push_back(ControlPoints.at(1));
			ControlPoints = tmp;
			return 1;
		}

		else {
			int minPosition;
			float  minDistance;
			minPosition = 0;
			minDistance = DistanceBetweenLineAndPoint(Point,
				ControlPoints.at(0),
				ControlPoints.at(1));

			for (int p = 1; p < ControlPoints.size(); p++) {
				float tmp = DistanceBetweenLineAndPoint(Point,
					ControlPoints.at(p - 1),
					ControlPoints.at(p));
				if (tmp < minDistance) {
					minDistance = tmp;
					minPosition = p;
				}
			}

			if (minPosition == ControlPoints.size() - 1)
			{
				minDistance = DistanceBetweenLineAndPoint(Point,
					ControlPoints.at(minPosition),
					ControlPoints.at(0));
				float tmp = DistanceBetweenLineAndPoint(Point,
					ControlPoints.at(minPosition),
					ControlPoints.at(minPosition - 1));
				if (tmp < minDistance) {
					minPosition -= 1;

				}
			}

			else if (minPosition == 0) {
				minDistance = DistanceBetweenLineAndPoint(Point,
					ControlPoints.at(minPosition),
					ControlPoints.at(minPosition + 1));
				float tmp = DistanceBetweenLineAndPoint(Point,
					ControlPoints.at(minPosition),
					ControlPoints.at(ControlPoints.size() - 1));
				if (tmp < minDistance) {
					minPosition = ControlPoints.size() - 1;
				}
			}

			else if (minPosition > 0) {
				minDistance = DistanceBetweenLineAndPoint(Point,
					ControlPoints.at(minPosition),
					ControlPoints.at(minPosition + 1));
				float tmp = DistanceBetweenLineAndPoint(Point,
					ControlPoints.at(minPosition),
					ControlPoints.at(minPosition - 1));
				if (tmp < minDistance) {
					minPosition = minPosition -= 1;
				}
			}

			std::vector<vec4> insertedCtrlPoints;

			for (int p = 0; p < ControlPoints.size() + 1; p++) {
				if (p <= minPosition)
					insertedCtrlPoints.push_back(ControlPoints.at(p));
				else if (p == minPosition + 1)
					insertedCtrlPoints.push_back(Point);
				else
					insertedCtrlPoints.push_back(ControlPoints.at(p - 1));
			}

			ControlPoints = insertedCtrlPoints;

			return minPosition + 1;

		}

	}

	std::vector<vec4> getConrtolPoints() { return ControlPoints; }

};


class CatmullRomSpline : public Poligon {
	std::vector<float> ts;

	vec4 Hermite(vec4 p0, vec4 v0, float t0, vec4 p1, vec4 v1, float t1, float t) {
		float deltat = t1 - t0;
		t -= t0;
		float deltat2 = deltat * deltat;
		float deltat3 = deltat * deltat2;
		vec4 a0 = p0, a1 = v0;
		vec4 a2 = (p1 - p0) * 3 / deltat2 - (v1 + v0 * 2) / deltat;
		vec4 a3 = (p0 - p1) * 2 / deltat3 + (v1 + v0) / deltat2;
		return ((a3 * t + a2) * t + a1) * t + a0;
	}
public:

	void AddControlPoint(float cX, float cY) {
		RecalculateTesselatedVertices();
		ts.push_back((float)ControlPoints.size());
		Poligon::AddControlPoint(cX, cY);
	}

	void AddControlPointWithOutTransformation(float cX, float cY) {
		RecalculateTesselatedVertices();
		ts.push_back((float)ControlPoints.size());
		Poligon::AddControlPointWithOutTransformation(cX, cY);
	}
	float tStart() { return ts.at(0); }
	float tEnd() { return ts.at(ControlPoints.size() - 1); }

	vec4 r(float t) {
		vec4 wPoint(0, 0);
		for (int i = 0; i <= ControlPoints.size() - 1; i++) {
			if (ts.at(i) <= t && t <= ts.at(i + 1)) {

				vec4 vPrev = (i > 0) ? (ControlPoints.at(i) - ControlPoints.at(i - 1)) * (1.0f / (ts.at(i) - ts.at(i - 1)))
					: (ControlPoints.at(i) - ControlPoints.at(ControlPoints.size() - 1) * (1.0f / (ts.at(i) - ts.at(ControlPoints.size() - 1))));

				vec4 vCur = (ControlPoints.at(i + 1) - ControlPoints.at(i)) / (ts.at(i + 1) - ts.at(i));

				vec4 vNext = (i < ControlPoints.size() - 2) ? (ControlPoints.at(i + 2) - ControlPoints.at(i + 1)) / (ts.at(i + 2) - ts.at(i + 1))
					: vec4(0, 0, 0, 0);

				vec4 v0 = (vPrev + vCur) * 1.0f;
				vec4 v1 = (vCur + vNext) * 1.0f;

				return Hermite(ControlPoints.at(i), v0, ts.at(i), ControlPoints.at(i + 1), v1, ts.at(i + 1), t);
			}
		}

		return ControlPoints.at(0);
	}

	void RecalculateTesselatedVertices() {
		nTesselatedVertices = 0;
		for (int i = 0; i < ControlPoints.size(); i++) {
			if (i == ControlPoints.size() - 1)
				nTesselatedVertices += 100 * round((abs(ControlPoints.at(i).x + ControlPoints.at(0).x) + abs(ControlPoints.at(i).x + ControlPoints.at(0).x)) / 2);
			else
				nTesselatedVertices += 100 * round((abs(ControlPoints.at(i).x + ControlPoints.at(i + 1).x) + abs(ControlPoints.at(i).y + ControlPoints.at(i + 1).y)) / 2);
		}
	}
};

Poligon* poligon;

void onInitialization() {

	glViewport(0, 0, windowWidth, windowHeight);
	glLineWidth(2.0f);

	poligon = new Poligon();


	gpuProgram.create(vertexSource, fragmentSource, "outColor");
}

void onDisplay() {
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	poligon->Draw();
	glutSwapBuffers();
}

void onKeyboard(unsigned char key, int pX, int pY) {
	animate = !animate;
	glutPostRedisplay();
}

void onKeyboardUp(unsigned char key, int pX, int pY) {
	if (key == 's') {
		std::vector<vec4> CtrlPoints = poligon->getConrtolPoints();
		if (!IsCRSpline) {
			poligon = new CatmullRomSpline();
			for (int i = 0; i < CtrlPoints.size(); i++)
				poligon->AddControlPointWithOutTransformation(CtrlPoints.at(i).x, CtrlPoints.at(i).y);
			IsCRSpline = true;

		}
		else {
			poligon = new Poligon();
			for (int i = 0; i < CtrlPoints.size(); i++)
				poligon->AddControlPointWithOutTransformation(CtrlPoints.at(i).x, CtrlPoints.at(i).y);
			IsCRSpline = false;
		}
	}
	else if (key == 'd') {
		poligon->DeletePoints();
	}

}

int pickedControlPoint = -1;

void onMouse(int button, int state, int pX, int pY) {
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		float cX = 2.0f * pX / windowWidth - 1;
		float cY = 1.0f - 2.0f * pY / windowHeight;
		poligon->AddControlPoint(cX, cY);
		glutPostRedisplay();
	}
	if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN) {
		float cX = 2.0f * pX / windowWidth - 1;
		float cY = 1.0f - 2.0f * pY / windowHeight;
		pickedControlPoint = poligon->PickControlPoint(cX, cY);
		glutPostRedisplay();
	}
	if (button == GLUT_RIGHT_BUTTON && state == GLUT_UP) {
		pickedControlPoint = -1;
	}
}


void onMouseMotion(int pX, int pY) {
	float cX = 2.0f * pX / windowWidth - 1;
	float cY = 1.0f - 2.0f * pY / windowHeight;
	if (pickedControlPoint >= 0) poligon->MoveControlPoint(pickedControlPoint, cX, cY);
}

void onIdle() {
	long time = glutGet(GLUT_ELAPSED_TIME);
	tCurrent = time / 1000.0f;
	glutPostRedisplay();
}