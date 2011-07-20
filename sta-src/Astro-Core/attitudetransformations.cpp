/*
 This program is free software; you can redistribute it and/or modify it under
 the terms of the European Union Public Licence - EUPL v.1.1 as published by
 the European Commission.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the European Union Public Licence - EUPL v.1.1
 for more details.

 You should have received a copy of the European Union Public Licence - EUPL v.1.1
 along with this program.

 Further information about the European Union Public Licence - EUPL v.1.1 can
 also be found on the world wide web at http://ec.europa.eu/idabc/eupl

*/

/*
 ------ Copyright (C) 2011 STA Steering Board (space.trajectory.analysis AT gmail.com) ----
*/

 //------------------ Author: Catarina Silva  -------------------------------------------------
// ------------------ E-mail: (catsilva20@gmail.com) ------------------------------------------

#include "attitudetransformations.h"
#include "Astro-core/RotationState.h"
#include "Astro-Core/stamath.h"
#include "Astro-core/stamath.h"
#include <Eigen/Core>
#include <Eigen/Geometry>

using namespace Eigen;

typedef Eigen::Matrix< double, 3, 3 > 	MyMatrix3d;
typedef Eigen::Matrix< double, 4, 4 > 	MyMatrix4d;
typedef Eigen::Matrix< double, 3, 1 > 	MyVector3d;

// Sets all the transformations between quaternions, Euler angles and Direction cosine matrix
// The following Euler sequences will be implemented: 123, 321, 313

//----------------------------------------------------------------------------------------------------------------------
// The matrices and formulas used are obtained from: http://gmat.gsfc.nasa.gov/. The quaternions are considered to have
// the following form: q = w + xi + yj + zk = q4 + q1i + q2j + q3k, with q4 being the real part, and q1, q2 and q3
// the imaginary part.
// Throughout the code, PHI is considered to be the first rotation angle, THETA the second rotation angle and PSI the
// third rotation angle.
//-----------------------------------------------------------------------------------------------------------------------
//Euler to Quaternions
/**
  * Converts the input Euler angles and sequence to a DCM
  *
  * @param EulerAngles  the input Euler angles (radians)
  * @param seq1         first entry of the Euler sequence
  * @param seq2         second entry of the Euler sequence
  * @param seq3         third entry of the Euler sequence
  *
  * @return the Quaternions
  */
Quaterniond ToQuaternions(const Vector3d &EulerAngles,
                                                int seq1,
                                                int seq2,
                                                int seq3)
{
    //Euler angles (phi, theta, psi)
    double phi=EulerAngles[0];
    double theta = EulerAngles[1];
    double psi = EulerAngles[2];

    // SEQUENCE 321
    if (seq1 == 3 && seq2 == 2 && seq3 == 1)
    {
//        static double RotCoeffs[9] = {
//            cos(theta)*cos(phi),                                cos(theta)*sin(psi),                               -sin(theta),
//            -cos(psi)*sin(phi)+sin(psi)*sin(theta)*cos(phi),    cos(phi)*cos(psi)+sin(psi)*sin(theta)*sin(phi),    sin(psi)*cos(theta),
//            sin(psi)*sin(phi)+cos(psi)*sin(theta)*cos(phi),     -sin(psi)*cos(phi)+cos(psi)*sin(theta)*sin(phi),   cos(psi)*cos(theta)
//        };
//        static const MyMatrix3d R_321(RotCoeffs);

//        //Convertion from DCM to Quaternions, for sequence 321
//        static Quaterniond quat(R_321);

//        return quat;
    Quaterniond quaternion = Quaterniond(AngleAxis<double> (phi,   Vector3d::UnitZ())*
                             Quaterniond(AngleAxis<double> (theta, Vector3d::UnitY()))*
                             Quaterniond(AngleAxis<double> (psi,   Vector3d::UnitX())));
    return quaternion;
    }

    // SEQUENCE 123
    if (seq1 == 1 && seq2 == 2 && seq3 == 3)
    {
//        static double RotCoeffs[9] = {
//            cos(psi)*cos(theta),    cos(psi)*sin(theta)*sin(phi)+sin(psi)*cos(phi),     -cos(psi)*sin(theta)*cos(phi)+sin(phi)*sin(psi),
//            -sin(psi)*cos(theta),   -sin(psi)*sin(theta)*sin(phi)+cos(psi)*cos(phi),    sin(psi)*sin(theta)*cos(phi)+cos(psi)*sin(phi),
//            sin(theta),             -cos(theta)*sin(phi),                               cos(theta)*cos(phi)
//        };
//        static const MyMatrix3d R_123(RotCoeffs);

//        //Convertion from DCM to Quaternions, for sequence 321
//        static Quaterniond quat(R_123);
        Quaterniond quaternion = Quaterniond(AngleAxis<double> (phi,   Vector3d::UnitX())*
                                 Quaterniond(AngleAxis<double> (theta, Vector3d::UnitY()))*
                                 Quaterniond(AngleAxis<double> (psi,   Vector3d::UnitZ())));
        return quaternion;

//        return quat;
    }

    // SEQUENCE 313
    if(seq1 == 3 && seq2 == 1 && seq3 == 3)
    {
//         static double RotCoeffs[9] = {
//            cos(psi)*cos(phi)-sin(psi)*cos(theta)*sin(phi),     cos(psi)*sin(phi)+sin(psi)*cos(theta)*cos(phi),     sin(psi)*sin(theta),
//            -sin(psi)*cos(phi)-cos(psi)*cos(theta)*sin(phi),    -sin(psi)*sin(phi)+cos(psi)*cos(theta)*cos(phi),    cos(psi)*sin(theta),
//            sin(theta)*sin(phi),                                -sin(theta)*cos(phi),                               cos(theta)
//         };
//         static const MyMatrix3d R_313(RotCoeffs);

//         //Convertion from DCM to Quaternions, for sequence 321
//         static Quaterniond quat(R_313);

//         return quat;
         Quaterniond quaternion = Quaterniond(AngleAxis<double> (phi,   Vector3d::UnitZ())*
                                  Quaterniond(AngleAxis<double> (theta, Vector3d::UnitX()))*
                                  Quaterniond(AngleAxis<double> (psi,   Vector3d::UnitZ())));
         return quaternion;

    }

}

//Quaternions to Euler angles
/**
  * Converts the input quaternions to a set of Euler angles, using the Euler sequence provided
  *
  * @param quaternion   the input quaternions
  * @param seq1         first entry of the Euler sequence
  * @param seq2         second entry of the Euler sequence
  * @param seq3         third entry of the Euler sequence
  *
  * @return the Euler angles (radians)
  */
Vector3d ToEulerAngles(Quaterniond &quaternion,
                                                int seq1,
                                                int seq2,
                                                int seq3)
{
//    //First convert quaternions to DCM
//    //Then, DCM to Euler
//    double q1 = quaternion.coeffs().coeffRef(0);
//    double q2 = quaternion.coeffs().coeffRef(1);
//    double q3 = quaternion.coeffs().coeffRef(2);
//    double q4 = quaternion.coeffs().coeffRef(3);
//    static double qCoeffs[3] = {q1, q2, q3};
//    static const Vector3d q(qCoeffs);

//    MyMatrix3d I = MyMatrix3d::Identity();

//    static double q_x_Coeffs[9] = {
//        0 ,  -q3,  q2,
//        q3,   0,  -q1,
//       -q2,   q1,   0
//    };
//    MyMatrix3d q_x(q_x_Coeffs);

//    double c = 1/ (q1*q1 + q2*q2 + q3*q3 +q4*q4);

//    double a = q.dot(q);
//    double b = q4*q4 - a;
//    MyMatrix3d d = b*I;
//    MatrixXd e = q.cross(q);
//    MyMatrix3d f = (q4*q_x);

//    MyMatrix3d R = c*(d + 2*e - 2*f);

////    MyMatrix3d R =c * ((q4*q4 - q.dot(q))*I + 2*q.cross(q) - 2*q4*q_x);

    //Transform the initial quaternions in the direction cosine matrix, using Eigen::Geometry capabilities.
    MyMatrix3d R = quaternion.toRotationMatrix();

    //Transform the direction cosine matrix in the Euler angles for the three sequences.
    // SEQUENCE 321
    if (seq1 == 3 && seq2 == 2 && seq3 == 1)
    {
            double a = R(1,2)/R(1,1);
        double theta1 = atan(a);
        double theta2 = asin(-R(1,3));
            double b = R(3,1)*sin(theta1) - R(3,2)*cos(theta1);
            double c = -R(2,1)*sin(theta1) + R(2,2)*cos(theta1);
        double theta3 = atan(b/c);

        Vector3d finalEulerAngles(theta1, theta2, theta3);
        return finalEulerAngles;
    }

    // SEQUENCE 123
    if(seq1 == 1 && seq2 == 2 && seq3 == 3)
    {
        double a = -R(3,2)/R(3,3);
    double theta1 = atan(a);
    double theta2 = asin(R(3,1));
        double b = R(1,3)*sin(theta1) + R(1,2)*cos(theta1);
        double c = R(2,3)*sin(theta1) + R(2,2)*cos(theta1);
    double theta3 = atan(b/c);

    Vector3d finalEulerAngles(theta1, theta2, theta3);
    return finalEulerAngles;
    }

    // SEQUENCE 313
    if(seq1 == 3 && seq2 == 1 && seq3 == 3)
    {
        double a = R(3,1)/(-R(3,2));
    double theta1 = atan(a);
    double theta2 = acos(R(3,3));
        double b = -R(2,2)*sin(theta1) - R(2,1)*cos(theta1);
        double c = R(1,2)*sin(theta1) + R(1,1)*cos(theta1);
    double theta3 = atan(b/c);

    Vector3d finalEulerAngles(theta1, theta2, theta3);
    return finalEulerAngles;
    }
}


//Body rates to Euler angle rates
/**
  * Converts the input angular velocity to a set of Euler angles rates, using the Euler sequence provided
  *
  * @param angVel       the input angular velocity (radians/seconds)
  * @param EulerAngles  the input Euler angles (radians)
  * @param seq1         first entry of the Euler sequence
  * @param seq2         second entry of the Euler sequence
  * @param seq3         third entry of the Euler sequence
  *
  * @return             the Euler angle rates representation (radians/second)
  */
Vector3d ToEulerAngleRates(const Vector3d &angVel,
                                     const Vector3d &EulerAngles,
                                     int seq1,
                                     int seq2,
                                     int seq3)
{
    //Convert the Euler angles from degress to radians
    double phi = sta::degToRad(EulerAngles[0]);
    double theta = sta::degToRad(EulerAngles[1]);
    double psi = sta::degToRad(EulerAngles[2]);

    // we need to check the singularities. theta = pi/2 there's a singularity
    bool singularity = false;

    // SEQUENCE 321
    if (seq1 == 3 && seq2 == 2 && seq3 == 1)
    {
        if(cos(theta)==0)
            singularity = true;
        else
        {
            static double MatrixCoeffs[9] = {
                  0.0,    sin(psi)/cos(theta),              cos(psi)/cos(theta),
                  0.0,    cos(psi),                         -sin(psi),
                  1.0,   (sin(psi)*sin(theta))/cos(theta),  (cos(psi)*sin(theta))/cos(theta)
            };
            static const MyMatrix3d Matrix(MatrixCoeffs);
            static const Vector3d finalEulerRates = Matrix*angVel;

            return finalEulerRates;
        }
    }

    // SEQUENCE 123
    if (seq1 == 1 && seq2 == 2 && seq3 == 3)
    {
        if(cos(theta)==0)
            singularity = true;
        else
        {
            static double MatrixCoeffs[9] = {
                cos(psi)/cos(theta),                    -sin(psi)/cos(theta),               0.0,
                sin(psi),                               cos(psi),                           0.0,
                (-cos(psi)*sin(theta))/cos(theta),      (sin(psi)*sin(theta))/cos(theta),   1.0
            };
            static const MyMatrix3d Matrix(MatrixCoeffs);
            static const Vector3d finalEulerRates = Matrix*angVel;

            return finalEulerRates;
        }
    }

    // SEQUENCE 313
    if (seq1 == 3 && seq2 == 1 && seq3 == 3)
    {
        if(sin(theta)==0)
            singularity = true;
        else
        {
            static double MatrixCoeffs[9] = {
                  sin(psi)/sin(theta),                  cos(psi)/sin(theta),                0.0,
                  cos(psi),                             -sin(psi),                          0.0,
                  -sin(psi)*cos(theta)/sin(theta),      -cos(psi)*cos(theta)/sin(theta),    1.0
            };
            static const MyMatrix3d Matrix(MatrixCoeffs);
            static const Vector3d finalEulerRates = Matrix*angVel;

            return finalEulerRates;
        }
    }
}



//Euler angle rates to Body rates
/**
  * Converts the input Euler angle rates to an angular velocity, using the Euler sequence provided
  *
  * @param EulerRates   the input Euler rates (radians/seconds)
  * @param EulerAngles  the input Euler angles (radians)
  * @param seq1         first entry of the Euler sequence
  * @param seq2         second entry of the Euler sequence
  * @param seq3         third entry of the Euler sequence
  *
  * @return             the angular velocity (radians/second)
  */
Vector3d ToAngularVelocity(const Vector3d &EulerRates,
                                     const Vector3d &EulerAngles,
                                     int seq1,
                                     int seq2,
                                     int seq3)
{
    //Convert the Euler angles from degress to radians
    double phi = sta::degToRad(EulerAngles[0]);
    double theta = sta::degToRad(EulerAngles[1]);
    double psi = sta::degToRad(EulerAngles[2]);

    // SEQUENCE 321
    if(seq1 == 3 && seq2 == 2 && seq3 == 1)
    {
        static double MatrixCoeffs[9] = {
                -sin(theta),                  0,          1,
                sin(psi)*cos(theta),          cos(psi),   0,
                cos(psi)*cos(theta),         -sin(psi),   0
        };
        static const MyMatrix3d Matrix(MatrixCoeffs);
        static const Vector3d finalAngVel = Matrix * EulerRates;
        return finalAngVel;
    }

    // SEQUENCE 123
    if(seq1 == 1 && seq2 == 2 && seq3 == 3)
    {
        static double MatrixCoeffs[9] = {
              cos(psi)*cos(theta),      sin(psi),   0.0,
              -sin(psi)*cos(theta),     cos(psi),   0.0,
              sin(theta),               0.0,        1.0
        };
        static const MyMatrix3d Matrix(MatrixCoeffs);
        static const Vector3d finalAngVel = Matrix * EulerRates;
        return finalAngVel;
    }

    // SEQUENCE 313
    if(seq1 == 3 && seq2 == 1 && seq3 == 3)
    {
        static double MatrixCoeffs[9] = {
            sin(psi)*sin(theta),        cos(psi),       0.0,
            cos(psi)*sin(theta),        -sin(psi),      0.0,
            cos(theta),                 0.0,            1.0
        };
        static const MyMatrix3d Matrix(MatrixCoeffs);
        static const Vector3d finalAngVel = Matrix * EulerRates;
        return finalAngVel;
    }
}



//------------------------------------------------------------------------------------------------------------
// These transformations were taken from: "Space vehicles dynamics and control", Bong Wie, AIAA Education Series
//------------------------------------------------------------------------------------------------------------

//Quaternion rates to body rates
Vector3d ToAngularVelocity(Eigen::Quaterniond &quaternion,
                                                      Eigen::Quaterniond &initQuatRates)
{
    // q = q4 + q1i + q2j + q3k
    double q1 = quaternion.coeffs().coeffRef(0);
    double q2 = quaternion.coeffs().coeffRef(1);
    double q3 = quaternion.coeffs().coeffRef(2);
    double q4 = quaternion.coeffs().coeffRef(3);

    double q1_Dot = initQuatRates.coeffs().coeffRef(0);
    double q2_Dot = initQuatRates.coeffs().coeffRef(1);
    double q3_Dot = initQuatRates.coeffs().coeffRef(2);
    double q4_Dot = initQuatRates.coeffs().coeffRef(3);

//    static const Vector qDot(q1_Dot,q2_Dot,q3_Dot, q4_Dot );

//    static const Vector3d angularRates = 2*W_q*qDot;
    double angRate_p = 2*(q4*q1_Dot + q3*q2_Dot - q2*q3_Dot - q1*q4_Dot);
    double angRate_q = 2*(-q3*q1_Dot + q4*q2_Dot + q1*q3_Dot - q2*q4_Dot);
    double angRate_r = 2*(q2*q1_Dot - q1*q2_Dot + q4*q3_Dot - q3*q4_Dot);

    Vector3d angularRates(angRate_p,angRate_q,angRate_r);
    return angularRates;

}

//Body rates to quaternion rates
Quaterniond ToQuaternionRates(Eigen::Quaterniond &quaternion,
                                                       Vector3d &bodyRates)
{

    double q1 = quaternion.coeffs().coeffRef(0);
    double q2 = quaternion.coeffs().coeffRef(1);
    double q3 = quaternion.coeffs().coeffRef(2);
    double q4 = quaternion.coeffs().coeffRef(3);

    double p = bodyRates[0];
    double q = bodyRates[1];
    double r = bodyRates[2];

    double q1_Dot = 0.5*(q4*p - q3*q + q2*r);
    double q2_Dot = 0.5*(q3*p + q4*q - q1*r);
    double q3_Dot = 0.5*(-q2*p + q1*q + q4*r);
    double q4_Dot = 0.5*(-q1*p - q2*q - q3*r);

    Quaterniond quatRates(q1_Dot, q2_Dot, q3_Dot, q4_Dot);
    return quatRates;
}