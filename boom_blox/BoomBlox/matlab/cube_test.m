% test script for generating sound for cube collision
%

% sampling frequency
fq = 44100;
dt = 1/fq;

% spring constant
%   k = (Young's Modulus) * (thickness);
% Young's Modulus for steel
Y = 200; 
% TODO: what thickness to use?
thickness = 0.10;

k = Y * thickness;


% mass of particles
%   m = density * thickness * area
% for now just assume homogenous object
% density for steel is 7.85 g/cm^3
mass = 0.785 * 3 * 0.1;

% fluid damping constant (must be negative)
gamma = -1;

% viscoelastic damping constant (must be negative)
n_eta = -1;

% vertices
vert = [ -1 -1  1;  ... p111
         -1 -1 -1;  ... p211
          1 -1  1;  ... p121
          1 -1 -1;  ... p221
         -1  1  1;  ... p112
         -1  1 -1;  ... p212
          1  1  1;  ... p122
          1  1 -1]; ... p222
 
% edges
%{
edges = [ 2 3 5;  ...
          4 6;    ...
          4 7;    ...
          8;      ...
          6 7;    ...
          8;      ...
          8]; 
%}

% number of vertices
n = size(vert,1);

% mass matrix
M = mass .* eye(n);
m = diag(M);

% K matrix (elastic force matrix)
%   TODO: this is only the 1D case (should be 3nx3n)
K = zeros(n);
K = [ 0 1 1 0 1 0 0 0 ; ...
      0 0 0 1 0 1 0 0 ; ...
      0 0 0 1 0 0 1 0 ; ...
      0 0 0 0 0 0 0 1 ; ...
      0 0 0 0 0 1 1 0 ; ...
      0 0 0 0 0 0 0 1 ; ...
      0 0 0 0 0 0 0 1 ; ...
      0 0 0 0 0 0 0 0 ];
K = K + K';
K = k*K;

% diagonalize K
[G D] = eig(K);
lambda = diag(D);
Ginv = inv(G);

% construct w vector
%   w_i = (-(gamma*lambda_i + n) +/- sqrt((gamma*lamba_i + n)^2 - 4*lambda_i))/2
t1 = -(gamma .* lambda + n_eta);
t2 = sqrt((gamma.*lambda + n_eta).^2 - 4.*lambda);
w_plus = (t1 + t2)./2.0;
w_minus = (t1 - t2)./2.0;

% intialize constant terms to 0;
c = zeros(n,1);

% force/impulse vector
%   This is given from collisions
f = zeros(n,1);
f(1) = 1;

% compute transformed impulse
g = Ginv * f;

% time of impact/collision
t0 = 0;

% initialize current time
t = 0;

% update constants
c = c + g./((m .* (w_plus - w_minus)) .* exp(w_plus * t0));
c_bar = conj(c);


% derive mode amplitude from the mode velocity
%   amplitude is related to the particle vecolicty which 
%   is linearly related to the mode velocity
% dz/dt = c * w_plus * exp(w_plus * t) + c_bar * w_minus * exp(w_minus * t)
vel = c .* w_plus .* exp(w_plus .* t) + c_bar .* w_minus .* exp(w_minus .* t);

% TODO: should there be some amplitude scaling factor? 
%         the relation between mode vel and particle
a = vel;









% for test purposes
c1 = c(1);
c1_bar = c_bar(1);
g1 = g(1);
m1 = m(1);
wp1 = w_plus(1);
wm1 = w_minus(1);
%v1 = c1 * wp1 * exp(wp1 * t) + c1_bar * wm1 * exp(wm1 * t);



