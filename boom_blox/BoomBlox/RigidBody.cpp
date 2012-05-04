#include "RigidBody.h"
#include <limits>
#include <gl/glut.h>
#include "Material.h"
#include <TestHarness.h>

#include <iostream>
#include <fstream>

namespace
{
	float inf = std::numeric_limits<float>::infinity();
}

RigidBody::RigidBody() :
	m_transformation(Matrix4::IDENTITY),
	m_velocity(Vector3::ZERO),
	m_angularVelocity(Vector3::ZERO),
	m_material(NULL),
	m_queuedDeltaVelocity(Vector3::ZERO),
	m_queuedDeltaAngularVelocity(Vector3::ZERO),
	m_hasInfiniteMass(false),
	id(0)
{
	
	// set 1: (metal bowl thingie) 0.1, 900, 0.00001, 0.1;
	// set 2: 0.005, 500, 0.00001, 0.1;
	// set 3: 5, 20, 0.00001, 0.1;
	// set 4 (lol): 0.1, 2000, 0.000000000000001, 0.0000000001;
	// set 5: (foot steps): 0.1, 20, 0.1, 0.1;
	// set 6: ("doh", don't know how to describe it, muted by water) 0.5, 1, 0.0001, 0.1;
	
	// tmp
	this->thickness = 0.1; 
	this->Y = 900;
	this->gamma = 0.00001;
	this->n_eta = 0.1;
}

void RigidBody::SetID(int id_)
{
	id = id_;
}

Material const* RigidBody::GetMaterial() const
{
	return m_material;
}

void RigidBody::SetMaterial(Material const* material)
{
	m_material = material;
	
	// set sound parameters
	this->thickness = material->thickness;
	this->Y = material->youngsModulus;
	this->gamma = material->fluidDamping;
	this->n_eta = material->viscoelasticDamping;

	initSoundScene();
}

Vector3 RigidBody::GetNormalLocalInverseInertialTensor() const
{
	Vector3 tensor = GetNormalLocalInertialTensor();
	return Vector3(1/tensor.x, 1/tensor.y, 1/tensor.z);
}

Vector3 RigidBody::GetPosition() const
{
	return m_transformation * Vector3::ZERO;
}

void RigidBody::SetPosition(Vector3 const& position)
{
	m_transformation.setTrans(position);
}

Quaternion RigidBody::GetOrientation() const
{
	return m_transformation.extractQuaternion();
}

void RigidBody::SetOrientation(Quaternion const& orientation)
{
	m_transformation.makeTransform(GetPosition(), Vector3::UNIT_SCALE, orientation);
}


Matrix4 const& RigidBody::GetTransformation() const
{
	return m_transformation;
}

void RigidBody::SetTransformation(Matrix4 const& transformation)
{
	m_transformation = transformation;
}

Vector3 const& RigidBody::GetVelocity() const
{
	return m_velocity;
}

void RigidBody::SetVelocity(Vector3 const& velocity)
{
	m_velocity = velocity;
}

Ogre::Vector3 RigidBody::GetVelocityAtPoint(Vector3 const& p) const
{
	Vector3 v = p-GetPosition();
	return GetAngularVelocity().crossProduct(v) + GetVelocity();
}

Vector3 const& RigidBody::GetAngularVelocity() const
{
	return m_angularVelocity;
}

void RigidBody::SetAngularVelocity(Vector3 const& angularVelocity)
{
	m_angularVelocity = angularVelocity;
}

void RigidBody::QueueImpulse(Vector3 const& impulse, Vector3 const& p)
{
	if(HasInfiniteMass()) return;

	Vector3 deltaVelocity = impulse * GetInverseMass();
	m_queuedDeltaVelocity += deltaVelocity;

	Vector3 deltaOmega = GetInverseInertialTensor() * (p-GetPosition()).crossProduct(impulse);
	m_queuedDeltaAngularVelocity += deltaOmega;
}

bool RigidBody::ApplyQueuedImpulses()
{
	if(HasInfiniteMass()) return false;

	bool significant = (m_queuedDeltaVelocity.squaredLength() > 1e-8 || m_queuedDeltaAngularVelocity.squaredLength() > 1e-8);

	if(significant)
	{
		SetVelocity(GetVelocity() + m_queuedDeltaVelocity);
		SetAngularVelocity(GetAngularVelocity() + m_queuedDeltaAngularVelocity);
	}


	m_queuedDeltaVelocity = Vector3::ZERO;
	m_queuedDeltaAngularVelocity = Vector3::ZERO;

	return significant;
}

void RigidBody::ApplyImpulse(Vector3 const& impulse, Vector3 const& point)
{
	if(HasInfiniteMass()) return;

	Vector3 deltaVelocity = impulse * GetInverseMass();
	SetVelocity(GetVelocity() + deltaVelocity);

	Vector3 deltaOmega = GetInverseInertialTensor() * (point-GetPosition()).crossProduct(impulse);
	SetAngularVelocity(GetAngularVelocity() + deltaOmega);
}

void RigidBody::SaveTransformation()
{
	m_savedTransformation = m_transformation;
}

void RigidBody::RestoreTransformation()
{
	m_transformation = m_savedTransformation;
}

void RigidBody::SaveVelocity()
{
	m_savedVelocity = m_velocity;
	m_savedAngularVelocity = m_angularVelocity;
}

void RigidBody::RestoreVelocity()
{
	m_velocity = m_savedVelocity;
	m_angularVelocity = m_savedAngularVelocity;
}

void RigidBody::AdvanceTransformation(float dT)
{
	Vector3 newPosition = GetPosition() + GetVelocity()*dT;
	float angularSpeed = GetAngularVelocity().length();
	Vector3 axis = GetAngularVelocity().normalisedCopy();
	Quaternion newOrientation = Quaternion(angularSpeed * dT, axis) * GetOrientation();
	newOrientation.normalise();

	m_transformation.makeTransform(newPosition, Vector3::UNIT_SCALE, newOrientation);
}

void RigidBody::AdvanceVelocity(Vector3 const& F, float dT)
{
	if(!HasInfiniteMass())
	{
		SetVelocity(GetVelocity() + F * dT);
	}
}

void RigidBody::SetInfiniteMass()
{
	m_hasInfiniteMass = true;
}

void RigidBody::UnsetInfiniteMass()
{
	m_hasInfiniteMass = false;
}

bool RigidBody::HasInfiniteMass() const
{
	return m_hasInfiniteMass;
}

Vector3 RigidBody::GetLocalInertialTensor() const
{
	return HasInfiniteMass() ? Vector3(inf, inf, inf) : GetNormalLocalInertialTensor();
}

Vector3 RigidBody::GetLocalInverseInertialTensor() const
{
	return HasInfiniteMass() ? Vector3(0, 0, 0) : GetNormalLocalInverseInertialTensor();
}

float RigidBody::GetMass() const
{
	return HasInfiniteMass() ? inf : GetNormalMass();
}

float RigidBody::GetInverseMass() const
{
	return HasInfiniteMass() ? inf : GetNormalInverseMass();
}

Matrix3 RigidBody::GetInertialTensor() const
{
	Vector3 diagonal = GetLocalInertialTensor();
	Matrix3 localTensor = Matrix3::ZERO;
	localTensor[0][0] = diagonal.x;
	localTensor[1][1] = diagonal.y;
	localTensor[2][2] = diagonal.z;

	Matrix3 rotation;
	GetTransformation().extract3x3Matrix(rotation);

	return rotation * localTensor * rotation.Transpose();
}

Matrix3 RigidBody::GetInverseInertialTensor() const
{
	Vector3 diagonal = GetLocalInverseInertialTensor();
	Matrix3 localInverseTensor = Matrix3::ZERO;
	localInverseTensor[0][0] = diagonal.x;
	localInverseTensor[1][1] = diagonal.y;
	localInverseTensor[2][2] = diagonal.z;

	Matrix3 rotation;
	GetTransformation().extract3x3Matrix(rotation);

	return rotation.Transpose() * localInverseTensor * rotation;
}

void RigidBody::Render() const
{
	glColor3f(GetMaterial()->color.x, GetMaterial()->color.y, GetMaterial()->color.z);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	
	Vector3 pos = GetPosition();
	glTranslatef(pos.x, pos.y, pos.z);
	float angle;
	Vector3 axis;
	GetOrientation().ToAngleAxis(angle, axis);
	glRotatef(angle*180.0f/M_PI, axis.x, axis.y, axis.z);

	DoRender();
	glPopMatrix();
	int err = glGetError();
	err = err;
}


void RigidBody::getK(Eigen::MatrixXf &K){
	int dimension = this->verticies.size(); // there are this many verticies
	if (dimension == 0) {
		return;
	}
	Eigen::MatrixXf K_Matrix = Eigen::MatrixXf::Zero(dimension, dimension); // creates matrix
	const Material *material = GetMaterial();

	for (int i = 0; i < dimension; i ++) {
		vector<Vertex *> neighbors = this->verticies.at(i)->getNeighbor();
		int a = this->verticies.at(i)->getId(); // a is first index
		for (int j = 0; j < (int)neighbors.size(); j++) {
			int b = neighbors.at(j)->getId(); // b is the neighbor's id

			// twice diagaonal 
			// positive K- constant
			K_Matrix(a, b) = K_Matrix(a, b) - 1; 
			K_Matrix(a, a) = K_Matrix(a, a) + 1;
			K_Matrix(b, b) = K_Matrix(b, b) + 1;
		}
	}
	
		
	// now the K is created for one dimention
	// so we need to make the K matrix 3N x 3N
	Eigen::MatrixXf K_expand = Eigen::MatrixXf::Zero(3*dimension, 3*dimension); 
	K_expand <<                                    K_Matrix, Eigen::MatrixXf::Zero(dimension, dimension), Eigen::MatrixXf::Zero(dimension, dimension),
              Eigen::MatrixXf::Zero(dimension, dimension),                                    K_Matrix, Eigen::MatrixXf::Zero(dimension, dimension),
              Eigen::MatrixXf::Zero(dimension, dimension), Eigen::MatrixXf::Zero(dimension, dimension),                                   K_Matrix;


	// now we need to multiply it by the k-constant
	// TODO: get material parameters from rigid body material (xml files)
	//float thickness = 0.01;
	//float thickness = m_material->thickness;
	// Young's modulus (http://en.wikipedia.org/wiki/Young%27s_modulus)
	//  steel - 200
	//float Y = 900;
	//float Y = material->youngsModulus;
	float k_constant = thickness * Y;

	K_expand = k_constant * K_expand;

	K = K_expand;
}

// returns the mass matrix
void RigidBody::getM(Eigen::VectorXf &M) {
	int dimension = this->verticies.size(); // there are this many verticies
	dimension = 3 * dimension;
	if (dimension == 0) {
		return;
	}
	
	// m = density * thickness * area; 
	// for now assume homogenous object
	// density for steel is 7.85 g/cm^3

	// TODO: get material parameters from rigid body material (xml files)
	//float thickness = 0.01;
	//float thickness = m_material->thickness;
	float mass = 0.785 * 3 * thickness;
	//float mass = m_material->pmass;

	M = mass * Eigen::VectorXf::Ones(dimension);
}

void RigidBody::diagonalizeK(const Eigen::MatrixXf &K, Eigen::MatrixXf &G,
                              Eigen::VectorXf &D, Eigen::MatrixXf &Ginv) {
	if (this->verticies.size() == 0) {
		return;
	}
	Eigen::SelfAdjointEigenSolver<Eigen::MatrixXf> eigensolver(K);
	if (eigensolver.info() != Eigen::Success) abort();

	// vector of eigenvalues
	D = eigensolver.eigenvalues();
	// eigenvectors corresponding to eigenvalues of D
	G = eigensolver.eigenvectors();
	Ginv = eigensolver.eigenvectors().inverse();
}

void RigidBody::constructW(Eigen::VectorXcf &W_plus, Eigen::VectorXcf &W_minus){
	// TODO: where should these parameters come from?
	// fluid damping
	//double gamma = 0.00001;
	//double gamma = m_material->fluidDamping;
	// viscolastic damping
	//double n_eta = 0.1;
	//double n_eta = m_material->viscoelasticDamping;

	int dimension = verticies.size();
	dimension = dimension * 3;
	if (dimension == 0) {
		return;
	}
	W_plus = Eigen::VectorXcf::Zero(dimension);
	W_minus = Eigen::VectorXcf::Zero(dimension);

	// imaginary numbers lol ... 
	// puts complex cast every where and hope it works

	// w_i = ( -(gamma*lambda_i + n) +/- sqrt ((gamma * lambda_i + n)^2 - 4*lambda_i))/2
	for (int i = 0; i < dimension; i ++) {
		double lambda_i = D(i);
		complex<float> p1 = -1.0f * (gamma * lambda_i + n_eta);
		complex<float> p2 = sqrt( complex<float> (pow(gamma * lambda_i + n_eta, 2) - 4 * lambda_i )); 
		
		W_plus(i) = complex<float> ((p1 + p2)/2.0f);
		W_minus(i) = complex<float> ((p1 - p2)/2.0f);
	}
}

void RigidBody::calcForce(Eigen::VectorXcf * force, Vector3 location, Vector3 impulse) {
	// get closest vertex, and get force?
	int dimension = verticies.size();
	float min_dist = (verticies.at(0)->getLocation() - location).length();
	float cur_dist = min_dist;
	int min_vertex = 0;
	for (int i = 0; i < dimension; i ++) {
		// if length is smaller update min_vertex
		if ( (cur_dist = (verticies.at(i) ->getLocation() - location).length()) < min_dist) {
			min_dist = cur_dist;
			min_vertex = i;
		}
	}

	// object space!!
	impulse = GetTransformation().inverse() * impulse;

	// after finding the vertex 
	// let's calc force matrix
	(*force)(min_vertex) = impulse[0];
	(*force)(dimension + min_vertex) = impulse[1];
	(*force)(2*dimension + min_vertex) = impulse[2];
}

void RigidBody::initSoundScene() {
	// call meshify
	this->meshify(5);
	// first calcualte K
	this->getK(K);
	// then get M
	this->getM(M);
	this->diagonalizeK(K, G, D, Ginv);
	// construct the W
	constructW(W_plus, W_minus);
}

void RigidBody::calculateSound(SoundManager *soundManager, Vector3 location, Vector3 impulse) {
	// only calculate sound if it is close enough
	// something like this? 
	float dist_fc = (GetTransformation().inverse() * location).length();
	if (dist_fc > 100) {
		return;
	} else if ( dist_fc > 70) {
		soundManager->setUserCreatedFrequency(8000);
	} else if ( dist_fc > 60) {
		soundManager->setUserCreatedFrequency(10000);
	} else if ( dist_fc > 50) {
		soundManager->setUserCreatedFrequency(15000);
	} else if ( dist_fc > 40) {
		soundManager->setUserCreatedFrequency(20000);
	} else if ( dist_fc > 30) {
		soundManager->setUserCreatedFrequency(30000);
	} else if ( dist_fc > 20) {
		soundManager->setUserCreatedFrequency(40000);
	} else {
		soundManager->setUserCreatedFrequency(44100);
	}
	
	

	// constants w00t
	float duration = 0.1;
	float fq = soundManager->GetUserCreatedFrequency();
	complex<float> dt = M_PI/100.0; // 1/fq;
	
	int dimension = verticies.size();

	// force/impulse passed from colissions
	Eigen::VectorXcf f = Eigen::VectorXcf::Zero(3 * dimension);
	calcForce(&f, location, impulse);

	// compute transformed impulse
	Eigen::VectorXcf g = Ginv * f;
	
	// time of impact / colission
	//	 We will independently calculate all sound responses so this is always zero?
	complex<float> t0 = 0;
	
	// init constant terms to 0
	Eigen::VectorXcf c = Eigen::VectorXcf::Zero(3 * dimension); 

	// update constants
	// c = c + g./((m .* (w_plus - w_minus)) .* exp(w_plus * t0));
	for (int i = 0; i < dimension * 3; i++) {
		c(i) = c(i) +  g(i) / (( M(i) * (W_plus(i) - W_minus(i))) * exp(W_plus(i) * t0));
	}
	Eigen::VectorXcf c_bar = c.conjugate();
	
	// current time
	complex<float> t = 0; // has to be complex 

	int nmode = dimension * 3;
	//printf("nmode: %d\n", nmode);
	int nsample = fq * duration;
	Eigen::VectorXf sample = Eigen::VectorXf::Zero(nsample);

	float max = 0;

	// summing modes
	/*************************************************
	// I am not sure why I thought the mode response would be calculated this way...
	//   it doesn't really make sense to have x_{t} = v * x_{t-1}
	//
	for (int i =0; i < nsample; i++) {
		complex<double> modes = 0; 
		t = M_PI * (i+1) / 100 ; //dt * (i+1);
		for (int j = 0; j < nmode; j++) {
			// mode_vel = @(t) c .* w_plus .* exp(w_plus .* t) + c_bar .* w_minus .* exp(w_minus .* t);
			complex<double> v = c(j) * W_plus(j) * exp ( W_plus(j) * t) + c_bar(j) * W_minus(j) * exp(W_minus(j) * t);
			//  mode_resp(:,ind) = v .* (c .* exp(w_plus .* t) + c_bar .* exp(w_minus .* t));
			modes = modes + v * (c(j) * exp(W_plus(j) *t) + c_bar(j) * exp (W_minus(j) * t));
		}
		sample(i) = (float)modes.real(); 

		if (abs(sample(i) > max)) {
			max = abs(sample(i));
		}
	}
	*************************************************/
	
	/*************************************************
	// vector to store the previous response for each mode
	Eigen::VectorXcf mode_t_minus_1 = Eigen::VectorXcf::Zero(3 * dimension);
	for (int i =0; i < nsample; i++) {
		t = dt * (i+1);
		
		for (int j = 0; j < nmode; j++) {
			// mode velocity
			//   mode_vel = @(t) c .* w_plus .* exp(w_plus .* t) + c_bar .* w_minus .* exp(w_minus .* t);
			complex<float> v = c(j) * W_plus(j) * exp ( W_plus(j) * t) + c_bar(j) * W_minus(j) * exp(W_minus(j) * t);
			
			// TODO: should this be v * dt?
			mode_t_minus_1(j) += v;

			// sum of all the modes for this sample
			sample(i) += mode_t_minus_1(j).real();
		}

		if (abs(sample(i)) > max) {
			max = abs(sample(i));
		}
	}
	*************************************************/

	// vector to store the previous response for each mode
	Eigen::VectorXcf mode_t_minus_1 = Eigen::VectorXcf::Zero(3 * dimension);
	Eigen::VectorXcf v_wplus = c.array() * W_plus.array();
	Eigen::VectorXcf v_wminus = c_bar.array() * W_minus.array();
	for (int i = 0; i < nsample; i++) {
		complex<float> t = dt * complex<float>(i+1);
		
		for (int j = 0; j < nmode; j++) {
			// mode velocity
			//   mode_vel = @(t) c .* w_plus .* exp(w_plus .* t) + c_bar .* w_minus .* exp(w_minus .* t);
			//complex<float> v = c(j) * W_plus(j) * exp ( W_plus(j) * t) + c_bar(j) * W_minus(j) * exp(W_minus(j) * t);
			
			// This should be slightly faster, 2 floating point complex multiplies instead of 6... still not real time
			//		v = c * w * e^(w * (t + dt)) == c * w * e^(w*t) * e^(w*dt)
			//		only need to multiply the previous value by e^(w*dt) to get the new velocity
			v_wplus(j) *= exp(W_plus(j) * dt);
			v_wminus(j) *= exp(W_minus(j) * dt);
			
			// TODO: should this be v * dt?
			mode_t_minus_1(j) += v_wplus(j) + v_wminus(j);

			// sum of all the modes for this sample
			sample(i) += mode_t_minus_1(j).real();
		}

		if (abs(sample(i)) > max) {
			max = abs(sample(i));
		}
	}
	
	// normalize it... 
	sample = sample/max;

	// play the sound
	soundManager->PlayUserCreatedSample(&sample(0), sample.size(), GetPosition(), GetVelocity());
}
