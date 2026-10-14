
pipeline {
    agent any

    options {
        timestamps()
    }

    environment {
        AWS_REGION = 'ap-south-1'
        ECR_REGISTRY = '618826682694.dkr.ecr.ap-south-1.amazonaws.com'
        ECR_REPOSITORY = 'routemaster-cca2'
        IMAGE_TAG = "${BUILD_NUMBER}"
    }

    stages {
        stage('Checkout') {
            steps {
                checkout scm
            }
        }

        stage('Build Docker Image') {
            steps {
                sh '''
                    docker build -t ${ECR_REGISTRY}/${ECR_REPOSITORY}:${IMAGE_TAG} .
                '''
            }
        }

        stage('Verify Image') {
            steps {
                sh '''
                    docker image inspect ${ECR_REGISTRY}/${ECR_REPOSITORY}:${IMAGE_TAG}
                '''
            }
        }

        stage('Login to Amazon ECR') {
            steps {
                sh '''
                    aws ecr get-login-password --region ${AWS_REGION} |
                    docker login --username AWS --password-stdin ${ECR_REGISTRY}
                '''
            }
        }

        stage('Push Image to ECR') {
            steps {
                sh '''
                    docker push ${ECR_REGISTRY}/${ECR_REPOSITORY}:${IMAGE_TAG}
                '''
            }
        }
    }

    post {
        success {
            echo 'RouteMaster image built and pushed to Amazon ECR successfully.'
        }
        failure {
            echo 'Pipeline failed. Check Console Output for the error.'
        }
        always {
            sh 'docker logout ${ECR_REGISTRY} || true'
        }
    }
}
